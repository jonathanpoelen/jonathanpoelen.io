#include <memory>
#include <cassert>

//BEGIN variant1
namespace detail
{
  class VariantBase;
}

template<class... Ts>
class Variant
{
public:
  Variant() = default;
  Variant(Variant&&) = default;
  Variant(Variant const&);

  template<class T>
  Variant(T&& x);

  Variant& operator=(Variant&&) = default;
  Variant& operator=(Variant const&);

  template<class T>
  Variant& operator=(T&& x);

  template<class F>
  auto visit(F&&);

private:
  std::unique_ptr<detail::VariantBase> impl_;
};
//END variant1

//BEGIN variant1_impl
namespace detail
{
  struct VariantBase
  {
    virtual std::unique_ptr<VariantBase> clone() = 0;
    virtual ~VariantBase() = default;
  };

  template<class T>
  struct VariantImpl : VariantBase
  {
    template<class U>
    VariantImpl(U&& x)
    : value_(std::forward<U>(x))
    {}

    std::unique_ptr<VariantBase> clone() override
    {
      return std::make_unique<VariantImpl>(value_);
    }

    T value_;
  };

  template<class T>
  auto make_variant_impl(T&& x)
  {
    return std::make_unique<VariantImpl<std::decay_t<T>>>(
      std::forward<T>(x));
  }
}

template<class... Ts>
Variant<Ts...>::Variant(Variant const& other)
: impl_(other.impl_->clone())
{}

template<class... Ts>
template<class T>
Variant<Ts...>::Variant(T&& x)
: impl_(detail::make_variant_impl(std::forward<T>(x)))
{}

template<class... Ts>
Variant<Ts...>& Variant<Ts...>::operator=(Variant const& other)
{
  impl_ = other.impl_ ? other.impl_->clone() : nullptr;
  return *this;
}

template<class... Ts>
template<class T>
Variant<Ts...>& Variant<Ts...>::operator=(T&& x)
{
  impl_ = detail::make_variant_impl(std::forward<T>(x));
  return *this;
}
//END variant1_impl

#if !defined(VIMPL) || VIMPL == 1
//BEGIN variant1_visit1
template<class... Ts>
template<class F>
auto Variant<Ts...>::visit(F&& f)
{
  assert(impl_);
  auto visit_impl = [&](auto* t){
    using Impl = detail::VariantImpl<std::decay_t<decltype(*t)>>;
    if (auto* p = dynamic_cast<Impl*>(impl_.get())) {
      f(p->value_);
    }
  };
  (visit_impl(static_cast<Ts*>(nullptr)), ...);
}
//END variant1_visit1
#else
//BEGIN variant1_visit2
template<class... Ts>
template<class F>
auto Variant<Ts...>::visit(F&& f)
{
  assert(impl_);
  auto visit_impl = [&](auto rec, auto* t, auto*... ts){
    using Impl = detail::VariantImpl<std::decay_t<decltype(*t)>>;
    if constexpr (sizeof...(ts)) {
      return dynamic_cast<Impl*>(impl_.get())
        ? f(static_cast<Impl*>(impl_.get())->value_)
        : rec(rec, ts...);
    }
    else {
      (void)rec;
      return f(static_cast<Impl*>(impl_.get())->value_);
    }
  };
  return visit_impl(visit_impl, static_cast<Ts*>(nullptr)...);
}
//END variant1_visit2
#endif

//BEGIN variant1_test
#include <iostream>

int main()
{
  using MyVariant = Variant<int, char const*>;
  MyVariant v;
  v = MyVariant{3};
  struct Visit{
    void operator()(int x) { std::cout << "int: " << x << '\n'; }
    void operator()(char const* x) { std::cout << "char*: " << x << '\n'; }
  };
  v.visit(Visit{});
  v = "plop";
  v.visit(Visit{});
}
//END variant1_test
