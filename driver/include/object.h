#pragma once

#include "driver/platform/platform.h"
#include "driver/utils/include/utils.h"
#include "driver/include/attributes.h"
#include "driver/include/diagnostics.h"

#include <fstream>
#include <memory>

class Object
    : public AttributeContainer
    , public DiagnosticsContainer
{
public:
    Object(const Object &) = delete;
    Object(Object &&) = delete;
    Object& operator= (const Object &) = delete;
    Object& operator= (Object &&) = delete;

    explicit Object() noexcept;

#if !defined(WORKAROUND_ALLOW_UNSAFE_DISPATCH)
    explicit Object(SQLHANDLE h) noexcept;
#endif

    virtual ~Object() = default;

    SQLHANDLE getHandle() const noexcept;

private:
    SQLHANDLE const handle;
};

class Driver;

template <typename Parent, typename Self>
class Child
    : public Object
    , public std::enable_shared_from_this<Self>
{
public:
    explicit Child(Parent & p) noexcept
        : parent(p)
    {
        getDriver().registerDescendant(getSelf());
    }

#if !defined(WORKAROUND_ALLOW_UNSAFE_DISPATCH)
    explicit Child(Parent & p, SQLHANDLE h) noexcept
        : Object(h)
        , parent(p)
    {
        getDriver().registerDescendant(getSelf());
    }
#endif

    virtual ~Child() {
        getDriver().unregisterDescendant(getSelf());
    }

    Driver & getDriver() const noexcept {
        return parent.getDriver();
    }

    Parent & getParent() const noexcept {
        return parent;
    }

    const Self & getSelf() const noexcept {
        return *static_cast<const Self *>(this);
    }

    Self & getSelf() noexcept {
        return *static_cast<Self *>(this);
    }

    void deallocateSelf() noexcept {
        parent.template deallocateChild<Self>(getHandle());
    }

    bool isLoggingEnabled() const {
        return parent.isLoggingEnabled();
    }

    std::ostream & getLogStream() {
        return parent.getLogStream();
    }

    void writeLogMessagePrefix(std::ostream & stream) {
        parent.writeLogMessagePrefix(stream);
        stream << "[" << getObjectTypeName<Self>() << "=" << toHexString(getHandle()) << "] ";
    }

private:
    Parent & parent;
};
