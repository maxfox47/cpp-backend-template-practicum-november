#include "use_cases_impl.h"

#include <algorithm>
#include <cctype>
#include <string>

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
    books_.SaveWithTags(entity, book.tags);
}

std::vector<AuthorDto> UseCasesImpl::GetAuthors() const {
    std::vector<AuthorDto> result;
    for (const auto& a : authors_.GetAll()) {
        result.push_back({a.GetId().ToString(), a.GetName()});
    }
    // Сортировка уже выполнена в SQL (ORDER BY LOWER(name), name), но для надежности сортируем еще раз
    // Используем case-insensitive сортировку для правильного порядка (как в SQL)
    std::sort(result.begin(), result.end(),
              [](const AuthorDto& a, const AuthorDto& b) {
                  std::string a_lower = a.name;
                  std::string b_lower = b.name;
                  std::transform(a_lower.begin(), a_lower.end(), a_lower.begin(), ::tolower);
                  std::transform(b_lower.begin(), b_lower.end(), b_lower.begin(), ::tolower);
                  if (a_lower != b_lower) {
                      return a_lower < b_lower;
                  }
                  return a.name < b.name;  // Если без учета регистра одинаковые, сравниваем с учетом регистра
              });
    return result;
}

std::vector<BookDto> UseCasesImpl::GetBooks() const {
    std::vector<BookDto> result;
    for (const auto& b : books_.GetAll()) {
        BookDto dto;
        dto.id = b.GetId().ToString();
        dto.author_id = b.GetAuthorId().ToString();
        auto author = authors_.GetById(b.GetAuthorId());
        if (author) {
            dto.author_name = author->GetName();
        }
        dto.title = b.GetTitle();
        dto.publication_year = b.GetPublicationYear();
        dto.tags = books_.GetTags(b.GetId());
        result.push_back(dto);
    }
    std::sort(result.begin(), result.end(), [](const BookDto& a, const BookDto& b) {
        if (a.title != b.title) {
            return a.title < b.title;
        }
        if (a.author_name != b.author_name) {
            return a.author_name < b.author_name;
        }
        return a.publication_year < b.publication_year;
    });
    return result;
}

std::vector<BookDto> UseCasesImpl::GetAuthorBooks(const std::string& author_id) const {
    std::vector<BookDto> result;
    for (const auto& b : books_.GetByAuthor(AuthorId::FromString(author_id))) {
        BookDto dto;
        dto.id = b.GetId().ToString();
        dto.author_id = b.GetAuthorId().ToString();
        auto author = authors_.GetById(b.GetAuthorId());
        if (author) {
            dto.author_name = author->GetName();
        }
        dto.title = b.GetTitle();
        dto.publication_year = b.GetPublicationYear();
        dto.tags = books_.GetTags(b.GetId());
        result.push_back(dto);
    }
    std::sort(result.begin(), result.end(), [](const BookDto& a, const BookDto& b) {
        if (a.publication_year != b.publication_year) {
            return a.publication_year < b.publication_year;
        }
        return a.title < b.title;
    });
    return result;
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    authors_.Delete(AuthorId::FromString(author_id));
}

void UseCasesImpl::EditAuthor(const std::string& author_id, const std::string& new_name) {
    authors_.Edit(AuthorId::FromString(author_id), new_name);
}

std::optional<AuthorDto> UseCasesImpl::GetAuthorByName(const std::string& name) const {
    auto author = authors_.GetByName(name);
    if (!author) {
        return std::nullopt;
    }
    return AuthorDto{author->GetId().ToString(), author->GetName()};
}

void UseCasesImpl::DeleteBook(const std::string& book_id) {
    books_.Delete(BookId::FromString(book_id));
}

void UseCasesImpl::EditBook(const std::string& book_id, const std::optional<std::string>& new_title,
                             const std::optional<int>& new_year,
                             const std::optional<std::vector<std::string>>& new_tags) {
    auto book = books_.GetById(BookId::FromString(book_id));
    if (!book) {
        throw std::runtime_error("Book not found");
    }
    std::string title = new_title.value_or(book->GetTitle());
    int year = new_year.value_or(book->GetPublicationYear());
    if (new_tags.has_value()) {
        // Если new_tags имеет значение (даже пустой вектор), обновляем теги
        books_.EditWithTags(book->GetId(), title, year, *new_tags);
    } else {
        // Если new_tags == std::nullopt, сохраняем существующие теги (не изменяем)
        books_.Edit(book->GetId(), title, year);
    }
}

std::vector<BookDto> UseCasesImpl::GetBooksByTitle(const std::string& title) const {
    std::vector<BookDto> result;
    for (const auto& b : books_.GetByTitle(title)) {
        BookDto dto;
        dto.id = b.GetId().ToString();
        dto.author_id = b.GetAuthorId().ToString();
        auto author = authors_.GetById(b.GetAuthorId());
        if (author) {
            dto.author_name = author->GetName();
        }
        dto.title = b.GetTitle();
        dto.publication_year = b.GetPublicationYear();
        dto.tags = books_.GetTags(b.GetId());
        result.push_back(dto);
    }
    
    return result;
}

std::optional<BookDto> UseCasesImpl::GetBookById(const std::string& book_id) const {
    auto book = books_.GetById(BookId::FromString(book_id));
    if (!book) {
        return std::nullopt;
    }
    BookDto dto;
    dto.id = book->GetId().ToString();
    dto.author_id = book->GetAuthorId().ToString();
    auto author = authors_.GetById(book->GetAuthorId());
    if (author) {
        dto.author_name = author->GetName();
    }
    dto.title = book->GetTitle();
    dto.publication_year = book->GetPublicationYear();
    dto.tags = books_.GetTags(book->GetId());
    return dto;
}

}  // namespace app

