#include "postgres.h"

#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(INSERT INTO authors (id, name) VALUES ($1, $2);)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
    pqxx::read_transaction tr{connection_};
    std::vector<domain::Author> authors;
    auto res = tr.exec(R"(SELECT id, name FROM authors ORDER BY LOWER(name), name;)"_zv);
    for (const auto& row : res) {
        authors.emplace_back(domain::AuthorId::FromString(row[0].c_str()),
                             row[1].c_str());
    }
    return authors;
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& author_id) {
    pqxx::work work{connection_};
    auto res = work.exec_params(
        R"(DELETE FROM authors WHERE id = $1;)"_zv,
        author_id.ToString());
    if (res.affected_rows() == 0) {
        throw std::runtime_error("Author not found");
    }
    work.commit();
}

void AuthorRepositoryImpl::Edit(const domain::AuthorId& author_id, const std::string& new_name) {
    pqxx::work work{connection_};
    auto res = work.exec_params(
        R"(UPDATE authors SET name = $1 WHERE id = $2;)"_zv,
        new_name, author_id.ToString());
    if (res.affected_rows() == 0) {
        throw std::runtime_error("Author not found");
    }
    work.commit();
}

std::optional<domain::Author> AuthorRepositoryImpl::GetById(const domain::AuthorId& author_id) const {
    pqxx::read_transaction tr{connection_};
    auto res = tr.exec_params(
        R"(SELECT id, name FROM authors WHERE id = $1;)"_zv,
        author_id.ToString());
    if (res.empty()) {
        return std::nullopt;
    }
    const auto& row = res[0];
    return domain::Author(domain::AuthorId::FromString(row[0].c_str()),
                          row[1].c_str());
}

std::optional<domain::Author> AuthorRepositoryImpl::GetByName(const std::string& name) const {
    pqxx::read_transaction tr{connection_};
    auto res = tr.exec_params(
        R"(SELECT id, name FROM authors WHERE name = $1;)"_zv,
        name);
    if (res.empty()) {
        return std::nullopt;
    }
    const auto& row = res[0];
    return domain::Author(domain::AuthorId::FromString(row[0].c_str()),
                          row[1].c_str());
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year)
VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(),
        book.GetPublicationYear());
    work.commit();
}

void BookRepositoryImpl::SaveWithTags(const domain::Book& book, const std::vector<std::string>& tags) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year)
VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(), book.GetAuthorId().ToString(), book.GetTitle(),
        book.GetPublicationYear());
    work.exec_params(R"(DELETE FROM book_tags WHERE book_id = $1;)"_zv, book.GetId().ToString());
    for (const auto& tag : tags) {
        work.exec_params(
            R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv,
            book.GetId().ToString(), tag);
    }
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
    pqxx::read_transaction tr{connection_};
    std::vector<domain::Book> books;
    auto res = tr.exec(
        R"(SELECT id, author_id, title, publication_year FROM books
           ORDER BY title, author_id, publication_year;)"_zv);
    for (const auto& row : res) {
        books.emplace_back(domain::BookId::FromString(row[0].c_str()),
                           domain::AuthorId::FromString(row[1].c_str()),
                           row[2].c_str(),
                           row[3].as<int>());
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthor(
    const domain::AuthorId& author_id) const {
    pqxx::read_transaction tr{connection_};
    std::vector<domain::Book> books;
    auto res = tr.exec_params(
        R"(SELECT id, author_id, title, publication_year FROM books
           WHERE author_id = $1
           ORDER BY publication_year, title;)"_zv,
        author_id.ToString());
    for (const auto& row : res) {
        books.emplace_back(domain::BookId::FromString(row[0].c_str()),
                           domain::AuthorId::FromString(row[1].c_str()),
                           row[2].c_str(),
                           row[3].as<int>());
    }
    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetByTitle(const std::string& title) const {
    pqxx::read_transaction tr{connection_};
    std::vector<domain::Book> books;
    auto res = tr.exec_params(
        R"(SELECT id, author_id, title, publication_year FROM books
           WHERE title = $1
           ORDER BY title, author_id, publication_year;)"_zv,
        title);
    for (const auto& row : res) {
        books.emplace_back(domain::BookId::FromString(row[0].c_str()),
                           domain::AuthorId::FromString(row[1].c_str()),
                           row[2].c_str(),
                           row[3].as<int>());
    }
    return books;
}

std::optional<domain::Book> BookRepositoryImpl::GetById(const domain::BookId& book_id) const {
    pqxx::read_transaction tr{connection_};
    auto res = tr.exec_params(
        R"(SELECT id, author_id, title, publication_year FROM books WHERE id = $1;)"_zv,
        book_id.ToString());
    if (res.empty()) {
        return std::nullopt;
    }
    const auto& row = res[0];
    return domain::Book(domain::BookId::FromString(row[0].c_str()),
                        domain::AuthorId::FromString(row[1].c_str()),
                        row[2].c_str(),
                        row[3].as<int>());
}

void BookRepositoryImpl::Delete(const domain::BookId& book_id) {
    pqxx::work work{connection_};
    auto res = work.exec_params(
        R"(DELETE FROM books WHERE id = $1;)"_zv,
        book_id.ToString());
    if (res.affected_rows() == 0) {
        throw std::runtime_error("Book not found");
    }
    work.commit();
}

void BookRepositoryImpl::Edit(const domain::BookId& book_id, const std::string& new_title, int new_year) {
    pqxx::work work{connection_};
    auto res = work.exec_params(
        R"(UPDATE books SET title = $1, publication_year = $2 WHERE id = $3;)"_zv,
        new_title, new_year, book_id.ToString());
    if (res.affected_rows() == 0) {
        throw std::runtime_error("Book not found");
    }
    work.commit();
}

void BookRepositoryImpl::EditWithTags(const domain::BookId& book_id, const std::string& new_title, int new_year, const std::vector<std::string>& tags) {
    pqxx::work work{connection_};
    auto res = work.exec_params(
        R"(UPDATE books SET title = $1, publication_year = $2 WHERE id = $3;)"_zv,
        new_title, new_year, book_id.ToString());
    if (res.affected_rows() == 0) {
        throw std::runtime_error("Book not found");
    }
    work.exec_params(R"(DELETE FROM book_tags WHERE book_id = $1;)"_zv, book_id.ToString());
    for (const auto& tag : tags) {
        work.exec_params(
            R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv,
            book_id.ToString(), tag);
    }
    work.commit();
}

void BookRepositoryImpl::SetTags(const domain::BookId& book_id, const std::vector<std::string>& tags) {
    pqxx::work work{connection_};
    work.exec_params(R"(DELETE FROM book_tags WHERE book_id = $1;)"_zv, book_id.ToString());
    for (const auto& tag : tags) {
        work.exec_params(
            R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv,
            book_id.ToString(), tag);
    }
    work.commit();
}

std::vector<std::string> BookRepositoryImpl::GetTags(const domain::BookId& book_id) const {
    pqxx::read_transaction tr{connection_};
    std::vector<std::string> tags;
    auto res = tr.exec_params(
        R"(SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag;)"_zv,
        book_id.ToString());
    for (const auto& row : res) {
        tags.push_back(row[0].c_str());
    }
    return tags;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
    author_id UUID NOT NULL REFERENCES authors(id) ON DELETE CASCADE,
    title varchar(100) NOT NULL,
    publication_year integer
);
)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID NOT NULL REFERENCES books(id) ON DELETE CASCADE,
    tag varchar(30) NOT NULL,
    PRIMARY KEY (book_id, tag)
);
)"_zv);
    work.commit();
}

}  // namespace postgres

