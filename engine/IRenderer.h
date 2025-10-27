#pragma once

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void Clear() = 0;
    virtual void Present() = 0;
};
