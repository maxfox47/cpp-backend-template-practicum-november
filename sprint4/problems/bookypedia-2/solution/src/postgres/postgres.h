#pragma once
#include <pqxx/pqxx>
#include <optional>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAll() const override;
    void Delete(const domain::AuthorId& author_id) override;
    void Edit(const domain::AuthorId& author_id, const std::string& new_name) override;
    std::optional<domain::Author> GetById(const domain::AuthorId& author_id) const override;
    std::optional<domain::Author> GetByName(const std::string& name) const override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Book& book) override;
    void SaveWithTags(const domain::Book& book, const std::vector<std::string>& tags) override;
    std::vector<domain::Book> GetAll() const override;
    std::vector<domain::Book> GetByAuthor(const domain::AuthorId& author_id) const override;
    std::vector<domain::Book> GetByTitle(const std::string& title) const override;
    std::optional<domain::Book> GetById(const domain::BookId& book_id) const override;
    void Delete(const domain::BookId& book_id) override;
    void Edit(const domain::BookId& book_id, const std::string& new_title, int new_year) override;
    void EditWithTags(const domain::BookId& book_id, const std::string& new_title, int new_year, const std::vector<std::string>& tags) override;
    void SetTags(const domain::BookId& book_id, const std::vector<std::string>& tags) override;
    std::vector<std::string> GetTags(const domain::BookId& book_id) const override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }
    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{connection_};
};

}  // namespace postgres

