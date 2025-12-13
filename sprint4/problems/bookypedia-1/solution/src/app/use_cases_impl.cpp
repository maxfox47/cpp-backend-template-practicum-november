#include "use_cases_impl.h"

#include <algorithm>

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("empty author name");
    }
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const BookDto& book) {
    if (book.title.empty()) {
        throw std::invalid_argument("empty title");
    }
    Book entity{BookId::New(), AuthorId::FromString(book.author_id), book.title,
                book.publication_year};
    books_.Save(entity);
}

std::vector<AuthorDto> UseCasesImpl::GetAuthors() const {
    std::vector<AuthorDto> result;
    for (const auto& a : authors_.GetAll()) {
        result.push_back({a.GetId().ToString(), a.GetName()});
    }
    std::sort(result.begin(), result.end(),
              [](const AuthorDto& a, const AuthorDto& b) { return a.name < b.name; });
    return result;
}

std::vector<BookDto> UseCasesImpl::GetBooks() const {
    std::vector<BookDto> result;
    for (const auto& b : books_.GetAll()) {
        result.push_back({b.GetId().ToString(), b.GetAuthorId().ToString(), b.GetTitle(),
                          b.GetPublicationYear()});
    }
    std::sort(result.begin(), result.end(),
              [](const BookDto& a, const BookDto& b) { return a.title < b.title; });
    return result;
}

std::vector<BookDto> UseCasesImpl::GetAuthorBooks(const std::string& author_id) const {
    std::vector<BookDto> result;
    for (const auto& b : books_.GetByAuthor(AuthorId::FromString(author_id))) {
        result.push_back({b.GetId().ToString(), b.GetAuthorId().ToString(), b.GetTitle(),
                          b.GetPublicationYear()});
    }
    std::sort(result.begin(), result.end(), [](const BookDto& a, const BookDto& b) {
        if (a.publication_year != b.publication_year) {
            return a.publication_year < b.publication_year;
        }
        return a.title < b.title;
    });
    return result;
}

}  // namespace app

