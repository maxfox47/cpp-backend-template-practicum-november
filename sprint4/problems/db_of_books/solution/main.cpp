#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

#include <iostream>
#include <optional>
#include <string>

using json = nlohmann::json;
using namespace std::literals;

constexpr auto kCreateTableSql =
    "CREATE TABLE IF NOT EXISTS books ("
    "id SERIAL PRIMARY KEY,"
    "title varchar(100) NOT NULL,"
    "author varchar(100) NOT NULL,"
    "year integer NOT NULL,"
    "ISBN char(13) UNIQUE"
    ");";

constexpr auto kInsertBookId = "ins_book";
constexpr auto kInsertBookSql =
    "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)";

constexpr auto kSelectAllBooksSql =
    "SELECT id, title, author, year, ISBN "
    "FROM books "
    "ORDER BY year DESC, title ASC, author ASC, ISBN ASC NULLS LAST";

struct AddBookPayload {
    std::string title;
    std::string author;
    int year;
    std::optional<std::string> isbn;
};

static AddBookPayload ParseAddPayload(const json& payload) {
    AddBookPayload data;
    data.title = payload.at("title").get<std::string>();
    data.author = payload.at("author").get<std::string>();
    data.year = payload.at("year").get<int>();
    if (payload.contains("ISBN") && !payload.at("ISBN").is_null()) {
        data.isbn = payload.at("ISBN").get<std::string>();
    }
    return data;
}

static void EnsureSchema(pqxx::connection& conn) {
    pqxx::work w{conn};
    w.exec(kCreateTableSql);
    // Гарантируем уникальность ISBN (NULL допускается)
    w.exec(
        "CREATE UNIQUE INDEX IF NOT EXISTS books_isbn_uindex ON books(ISBN);");
    w.commit();
}

static void PrepareStatements(pqxx::connection& conn) {
    // Готовим вставку; если уже существует, игнорируем ошибку
    try {
        conn.prepare(kInsertBookId, kInsertBookSql);
    } catch (const std::exception&) {
        // prepared statement мог быть уже создан — этого достаточно
    }
}

static bool HandleAddBook(pqxx::connection& conn, const json& payload) {
    auto data = ParseAddPayload(payload);
    try {
        pqxx::work w{conn};
        std::optional<std::string> isbn_param = data.isbn;
        w.exec_prepared(kInsertBookId, data.title, data.author, data.year,
                        isbn_param);
        w.commit();
        return true;
    } catch (const pqxx::sql_error&) {
        return false;
    } catch (...) {
        return false;
    }
}

static json HandleAllBooks(pqxx::connection& conn) {
    pqxx::read_transaction r{conn};
    json result = json::array();
    for (auto [id, title, author, year, isbn] :
         r.query<std::int32_t, std::string, std::string, int,
                 std::optional<std::string>>(kSelectAllBooksSql)) {
        json item;
        item["id"] = id;
        item["title"] = title;
        item["author"] = author;
        item["year"] = year;
        if (isbn.has_value()) {
            item["ISBN"] = *isbn;
        } else {
            item["ISBN"] = nullptr;
        }
        result.push_back(std::move(item));
    }
    return result;
}

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: book_manager <conn-string>\n";
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n";
            return EXIT_FAILURE;
        }

        pqxx::connection conn{argv[1]};
        EnsureSchema(conn);
        PrepareStatements(conn);

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                continue;
            }
            json request = json::parse(line);
            const auto action = request.at("action").get<std::string>();
            const auto& payload = request.at("payload");

            if (action == "add_book") {
                const bool ok = HandleAddBook(conn, payload);
                json resp;
                resp["result"] = ok;
                std::cout << resp.dump() << std::endl;
            } else if (action == "all_books") {
                json resp = HandleAllBooks(conn);
                std::cout << resp.dump() << std::endl;
            } else if (action == "exit") {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

