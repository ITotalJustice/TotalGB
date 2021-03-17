#pragma once

#include <cstdint>
#include <utility> // std::forward
// #include <compare>

namespace mem {

// this isn't a full replacement for std::shared_ptr
// it only has the stuff that i need, and more stuff will be
// added as needed.
// there's simple ref counting with copy and move constructors
// this is NOT thread safe as ref counting is not atmoic!

// this impl is basically a copy of the impl found here, thanks!
// SOURCE: https://medium.com/analytics-vidhya/c-shared-ptr-and-how-to-write-your-own-d0d385c118ad
template <typename T>
class shared_ptr {
public:
    constexpr shared_ptr()
        : ptr{nullptr}
        , ref_count(new std::size_t(1)) {
    }

    constexpr shared_ptr(T* existing_ptr)
        : ptr{existing_ptr}
        , ref_count(new std::size_t(1)) {
    }

    constexpr ~shared_ptr() {
        this->cleanup();
    }

// copy stuff
    // copy constructor
    constexpr shared_ptr(const shared_ptr& obj) noexcept {
        this->ptr = obj.ptr; // share the underlying pointer
        this->ref_count = obj.ref_count; // share ref_count
        
        if (this->ptr != nullptr) {
            // if the pointer is not null, increment the ref_count
            this->increment_ref();
        }
    }

    // copy assignment
    constexpr auto operator=(const shared_ptr& obj) noexcept -> shared_ptr& {
        // cleanup any existing data    // Assign incoming object's data to this object
        this->cleanup();

        this->ptr = obj.ptr; // share the underlying pointer
        this->ref_count = obj.ref_count; // share ref_count

        if (this->ptr != nullptr) {
            this->increment_ref();
        }

        return *this;
    }

// move stuff
    // move constructor
    constexpr shared_ptr(shared_ptr&& moved_obj) noexcept {
        this->ptr = moved_obj.ptr; // share the underlying pointer
        this->ref_count = moved_obj.ref_count; // share ref_count

        // cleanup moved obj
        moved_obj.ptr = nullptr;
        moved_obj.ref_count = nullptr;
    }

    // move assignment
    constexpr auto operator=(shared_ptr&& moved_obj) noexcept -> shared_ptr& {
        // cleanup any existing data
        this->cleanup();

        this->ptr = moved_obj.ptr; // share the underlying pointer
        this->ref_count = moved_obj.ref_count; // share ref_count

        // clean up moved_obj
        moved_obj.ptr = nullptr;
        moved_obj.ref_count = nullptr;

        return *this;
    }

// helpers
    [[nodiscard]]
    constexpr const auto get() const noexcept {
        return this->ptr;
    }

    [[nodiscard]]
    constexpr auto get() noexcept {
        return this->ptr;
    }

// operator overloads
    constexpr T* operator->() const noexcept {
        return this->get();
    }
    
    constexpr T& operator*() const noexcept {
        return this->get();
    }

    // constexpr auto operator<=>(std::nullptr_t) const noexcept {
    //     return this->get() <=> nullptr;
    // }

    constexpr explicit operator bool() const noexcept {
        return  this->get() != static_cast<T*>(nullptr);
    }

private:
    T* ptr{nullptr};
    std::size_t* ref_count{nullptr};

private:
    constexpr auto get_ref() const noexcept {
        return *this->ref_count;
    }

    constexpr void increment_ref() noexcept {
        ++*this->ref_count;
    }

    constexpr void decrement_ref() noexcept {
        --*this->ref_count;
    }

    constexpr void cleanup() noexcept {
        if (this->ref_count != nullptr) {
            this->decrement_ref();

            if (this->get_ref() == 0) {
                delete ref_count;
                this->ref_count = nullptr;

                if (this->ptr != nullptr) {
                    delete ptr;
                    this->ptr = nullptr;
                }
            }
        }
    }
};

// NOTE: this still does 2 allocs rather than 1.
// i'll correct this later.
template <typename T, typename... Args> [[nodiscard]]
constexpr auto make_shared(Args&&... args) -> shared_ptr<T> {
    return shared_ptr<T>{
        new T{std::forward<Args>(args)...}
    };
}

} // namespace mem
