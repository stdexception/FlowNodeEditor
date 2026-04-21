#include "nodeeditor/EdgeValidators.hpp"
#include "GraphicsSocket.hpp"
#include "GraphicsNode.hpp"

#include <QDebug>

namespace nodeeditor {

static bool s_defaultsRegistered = false;

void EdgeValidators::registerDefaults()
{
    if (s_defaultsRegistered)
        return;
    s_defaultsRegistered = true;
}

bool EdgeValidators::twoOutputsOrTwoInputs(GraphicsSocket* a, GraphicsSocket* b)
{
    if (!a || !b)
        return false;
    const bool aIn = a->isInput();
    const bool bIn = b->isInput();
    if (!aIn && !bIn) {
        qWarning() << "Edge validation: cannot connect two outputs";
        return false;
    }
    if (aIn && bIn) {
        qWarning() << "Edge validation: cannot connect two inputs";
        return false;
    }
    return true;
}

bool EdgeValidators::sameNode(GraphicsSocket* a, GraphicsSocket* b)
{
    if (!a || !b)
        return false;
    if (a->node() == b->node()) {
        qWarning() << "Edge validation: cannot connect sockets on the same node";
        return false;
    }
    return true;
}

bool EdgeValidators::sameSocketType(GraphicsSocket* a, GraphicsSocket* b)
{
    if (!a || !b)
        return false;
    if (a->dataType() != b->dataType()) {
        qWarning() << "Edge validation: socket types differ";
        return false;
    }
    return true;
}

bool EdgeValidators::validate(GraphicsSocket* a, GraphicsSocket* b)
{
    registerDefaults();
    if (!a || !b)
        return false;
    if (!twoOutputsOrTwoInputs(a, b))
        return false;
    if (!sameNode(a, b))
        return false;
    if (!sameSocketType(a, b))
        return false;
    return true;
}

} // namespace nodeeditor
