#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>

#ifdef STUPIDLY_STD_COMPLIANT
#  include <iterator>
#else
namespace std {
    struct bidirectional_iterator_tag;
}
#endif

#define NDEBUG 1;

namespace Svm {

    struct intrusive_node;
    namespace details {
        template<typename T1, typename T2>
/*constexpr*/ size_t offset_of(T1 T2::*mem_p);

        class ilist_base;
    } // namespace details

    struct intrusive_node {
    private:
        friend details::ilist_base;
        intrusive_node *next_{nullptr};
        intrusive_node *prev_{nullptr};

        constexpr void remove_self() {
            intrusive_node *prev_node = get_prev();
            intrusive_node *next_node = get_next();

            if (prev_node != nullptr) {
                prev_node->set_next(next_node);
            }
            if (next_node != nullptr) {
                next_node->set_prev(prev_node);
            }
            set_next(nullptr);
            set_prev(nullptr);
        }

    public:
        constexpr intrusive_node() noexcept = default;

        constexpr intrusive_node(const intrusive_node &other) = delete;

        // no constexpr std::exchange, sad.
        constexpr intrusive_node(intrusive_node &&other) noexcept {
            // TODO
            *this = std::move(other);
        }

        ~intrusive_node() {
            if (!is_linked()) {
                return;
            }
            remove_self();
        }

        constexpr intrusive_node &operator=(const intrusive_node &) = delete;

        constexpr intrusive_node &operator=(intrusive_node &&other) noexcept {
            if (&other == this) {
                return *this;
            }
            set_next(other.get_next());
            set_prev(other.get_prev());
            other.set_next(nullptr);
            other.set_prev(nullptr);
            if (get_next()) {
                get_next()->set_prev(this);
            }
            if (get_prev()) {
                get_prev()->set_next(this);
            }
            assert(!other.is_linked());
            return *this;
        }

        // I can't do this, it becomes non-literal.
        //  ~intrusive_node() { assert(!is_linked() && "destructor called on linked node."); }

        constexpr intrusive_node *get_next() { return next_; }

        constexpr intrusive_node *get_next() const { return next_; }

        constexpr void set_next(intrusive_node *n) {
            assert(n != this && "attempted to link to self.");
            next_ = n;
        }

        constexpr intrusive_node *get_prev() { return prev_; }

        constexpr intrusive_node *get_prev() const { return prev_; }

        constexpr void set_prev(intrusive_node *n) {
            assert(n != this && "attempted to link to self.");
            prev_ = n;
        }

        constexpr bool is_linked() const {
            intrusive_node *prev_node = get_prev();
            intrusive_node *next_node = get_next();
            return (prev_node != nullptr) || (next_node != nullptr);
        }

        constexpr bool is_head() const {
            intrusive_node *prev_node = get_prev();
            return (prev_node != nullptr);
        }

        constexpr bool is_tail() const {
            intrusive_node *next_node = get_next();
            return (next_node != nullptr);
        }

        // ?? should this return const T*?
        template<typename T, intrusive_node T::*mem_p>
        const T *owner() const {
            const char *this_addr = reinterpret_cast<const char *>(this);
            // itanium abi only
            // static constexpr size_t this_offset = details::offset_of(mem_p);
#if __GNUC__ && !_WIN32
            static_assert(sizeof(mem_p) == sizeof(ptrdiff_t));
            ptrdiff_t this_offset = 0;
            static constexpr intrusive_node T::*mem_p2 = mem_p;
            // itanium abi stores pointers to members as just an offset with the size of a ptrdiff_t.
            std::memcpy(&this_offset, &mem_p2, sizeof(mem_p2));
            assert(this_offset >= 0 && "given a bad member pointer?");
#else
            // No idea if this will be optimized away like the above is.
            size_t this_offset = details::offset_of(mem_p);
#endif
            const char *owner_addr = this_addr - this_offset;
            assert(owner_addr <= this_addr);
            assert(reinterpret_cast<std::uintptr_t>(owner_addr) % alignof(T) == 0);
            return reinterpret_cast<const T *>(owner_addr);
        }

        template<typename T, intrusive_node T::*mem_p>
        T *owner() {
            return const_cast<T *>(const_cast<const intrusive_node *>(this)->owner<T, mem_p>());
        }
    };

    namespace details {
        struct list_empty_t {
        };

        template<typename T1, typename T2>
/*constexpr*/ size_t offset_of(T1 T2::*mem_p) {
            union U {
                char data[sizeof(T2)];
                T2 v;

                ~U() {}
            } u{};

            // workaround for gcc bug
            bool found = false;
            // https://stackoverflow.com/a/49776289/754018
            const void *const desired_addr = static_cast<void const *>(std::addressof(u.v.*mem_p));
            for (size_t i = 0; i != sizeof(T2); ++i) {
                const void *const checking_addr = static_cast<void const *>(std::addressof(u.data[i]));
                if (checking_addr == desired_addr) {
                    return i;
                }
            }
            if (!found) {
                throw 0;
            }
            return -1u;
        }

    } // namespace details

    template<typename T, intrusive_node T::*node_ptr, bool isConst = false>
    class list_iterator {
    public:
        using value_type = std::conditional_t<isConst, const T, T>;
        using pointer = value_type *;
        using reference = value_type &;
        using difference_type = std::ptrdiff_t;
//        using iterator_category = std::bidirectional_iterator_tag;

        using node = std::conditional_t<isConst, const intrusive_node, intrusive_node>;
        node *ptr_;

        explicit list_iterator(node *ptr) : ptr_(ptr) {}

        reference operator*() {
            assert(ptr_ != nullptr);
            return *(ptr_->template owner<T, node_ptr>());
        }

        pointer operator->() { return ptr_->template owner<T, node_ptr>(); }

        list_iterator &operator++() {
            assert(ptr_);
            ptr_ = ptr_->get_next();
            return *this;
        }

        list_iterator operator++(int) {
            operator++();
            return *this;
        }

        list_iterator &operator--() {
            assert(ptr_);
            ptr_ = ptr_->get_prev();
            return *this;
        }

        list_iterator operator--(int) {
            operator--();
            return *this;
        }

        constexpr difference_type operator-(const list_iterator &rhs) { return ptr_ - rhs.ptr_; }

        constexpr bool operator==(const list_iterator &rhs) const { return ptr_ == rhs.ptr_; }

        constexpr bool operator!=(const list_iterator &rhs) const { return *this != rhs; }

        constexpr bool operator<(const list_iterator &rhs) const { return (rhs - *this) > 0; }

        constexpr bool operator>(const list_iterator &rhs) const { return rhs < *this; }

        constexpr bool operator<=(const list_iterator &rhs) const { return *this <= rhs; }

        constexpr bool operator>=(const list_iterator &rhs) const { return *this >= rhs; }
    };

    namespace details {
        class ilist_base {
        public:
            using difference_type = std::ptrdiff_t;
            using size_type = std::size_t;

            inline ilist_base() noexcept;

            ilist_base(const ilist_base &) = delete;

            ilist_base &operator=(const ilist_base &) = delete;

            inline ilist_base(ilist_base &&other) noexcept;

            inline ilist_base &operator=(ilist_base &&) noexcept;

            inline ~ilist_base();

            [[nodiscard]] inline bool empty() const;

            [[nodiscard]] inline bool is_empty() const;

            [[nodiscard]] inline size_type size() const;

            inline void insert_after(intrusive_node &pos, intrusive_node &val);

            inline void pop_front();

            inline void pop_back();

            inline void erase(intrusive_node *n);

            inline void clear();

        protected:
            inline void node_invariant(intrusive_node *n) const;

            inline void modification_invariant() const;

            intrusive_node head_{};
            intrusive_node tail_{};
            size_type size_{};

        private:
            inline void move_from(ilist_base &other) noexcept;
        };

        inline ilist_base::ilist_base() noexcept {
            head_.set_next(std::addressof(tail_));
            tail_.set_prev(std::addressof(head_));
        }

        inline void ilist_base::move_from(ilist_base &other) noexcept {
            if (other.empty()) {
                head_.set_next(std::addressof(tail_));
                tail_.set_prev(std::addressof(head_));
            } else {
                head_.set_next(other.head_.get_next());
                head_.get_next()->set_prev(&head_);
                tail_.set_prev(other.tail_.get_prev());
                tail_.get_prev()->set_next(&tail_);
            }
            other.head_.set_next(&other.tail_);
            other.tail_.set_prev(&other.head_);
            assert(other.empty());
        }

        inline ilist_base::ilist_base(ilist_base &&other) noexcept {
            move_from(other);
        }

        inline auto ilist_base::operator=(ilist_base &&other) noexcept -> ilist_base & {
            clear();
            move_from(other);
            return *this;
        }

        inline ilist_base::~ilist_base() {
            if (empty()) {
                return;
            }
            clear();
        }

        inline bool ilist_base::empty() const {
            return is_empty();
        }

        inline bool ilist_base::is_empty() const {
            return head_.get_next() == &tail_;
        }

        inline ilist_base::size_type ilist_base::size() const {
            return size_;
        }

        inline void ilist_base::insert_after(intrusive_node &pos, intrusive_node &val) {
            node_invariant(&val);
            node_invariant(&pos);
            ++size_;
            assert(!val.is_linked() && "this node is already part of a list.");

            intrusive_node &next = *pos.get_next();
            assert(next.get_prev() == &pos && "sanity error");
            val.set_next(&next);
            val.set_prev(&pos);
            pos.set_next(&val);
            next.set_prev(&val);
        }

        inline void ilist_base::pop_front() {
            intrusive_node *real_head = head_.get_next();
            assert(real_head->is_linked());
            erase(real_head);
        }

        inline void ilist_base::pop_back() {
            intrusive_node *real_tail = tail_.get_prev();
            assert(real_tail->is_linked());
            erase(real_tail);
        }

        inline void ilist_base::erase(intrusive_node *n) {
            modification_invariant();
            assert(n != &head_ && "Invalid node.");
            assert(n != &tail_ && "Invalid node.");
            node_invariant(n);
            n->remove_self();
            node_invariant(n);
            --size_;
        }

        inline void ilist_base::clear() {
            intrusive_node *n = head_.get_next();
            while (n != &tail_) {
                intrusive_node *prev = n;
                n = n->get_next();
                prev->remove_self();
            }
            head_.set_next(&tail_);
            tail_.set_prev(&head_);
        }

        inline void ilist_base::node_invariant(intrusive_node *n) const {
#ifndef NDEBUG
            if (!n->is_linked()) {
                assert(n->get_next() == nullptr);
                assert(n->get_prev() == nullptr);
                return;
            }
            if (n != &tail_) {
                assert(n->get_next() != nullptr && "sanity error");
                assert(n->get_next()->get_prev() == n && "sanity error");
            }
            if (n != &head_) {
                assert(n->get_prev() != nullptr && "sanity error");
                assert(n->get_prev()->get_next() == n && "sanity error");
            }
#else
            static_cast<void>(n);
#endif
        }

        inline void ilist_base::modification_invariant() const {
#ifndef NDEBUG
            assert(tail_.get_prev() != nullptr);
            assert(head_.get_next() != nullptr);
#endif
        }
    } // namespace details

    template<typename T, intrusive_node T::*node_ptr = T::intrusive_node>
    class intrusive_list : public details::ilist_base {
        using details::ilist_base::erase;

    public:
        using details::ilist_base::insert_after;

        using value_type = T;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using difference_type = std::ptrdiff_t;

        using iterator = Svm::list_iterator<T, node_ptr>;
        using const_iterator = Svm::list_iterator<T, node_ptr, true>;

        reference front() {
            assert(!empty());
            return *(head_.get_next()->template owner<T, node_ptr>());
        }

        const_reference front() const {
            assert(!empty());
            return *(head_.get_next()->template owner<T, node_ptr>());
        }

        reference back() {
            assert(!empty());
            return *(tail_.get_prev()->template owner<T, node_ptr>());
        }

        const_reference back() const {
            assert(!empty());
            return *(tail_.get_prev()->template owner<T, node_ptr>());
        }

        void push_back(reference val) {
            intrusive_node *real_tail = tail_.get_prev();
            assert(real_tail->is_linked() && "sanity error");
            insert_after(real_tail->owner<T, node_ptr>(), val);
        }

        void push_front(reference val) { insert_after(head_, val.*node_ptr); }

        void insert_after(pointer pos, reference val) {
            modification_invariant();
            assert(pos != nullptr && "can't insert after a null pointer.");

            intrusive_node &pos_node = pos->*node_ptr;
            intrusive_node &n = val.*node_ptr;
            return insert_after(pos_node, n);
        }

        void insert_after(const_iterator pos, reference val) {
            // can't insert anything after `end`.
            assert(pos != end());
            intrusive_node &pos_node = (*pos).*node_ptr;
            intrusive_node &n = val->*node_ptr;
            return insert_after(pos_node, n);
        }

        void erase(iterator pos) {
            intrusive_node *n = &((*pos).*node_ptr);
            erase(n);
        }

        void erase(reference pos) {
            intrusive_node *n = &(pos.*node_ptr);
            erase(n);
        }

        iterator begin() { return iterator{head_.get_next()}; }

        const_iterator begin() const { return const_iterator{head_.get_next()}; }

        const_iterator cbegin() const { return begin(); }

        iterator end() { return iterator{&tail_}; }

        const_iterator end() const { return const_iterator{&tail_}; }

        const_iterator cend() const { return end(); }
    };
} // namespace Svm
