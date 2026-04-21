#pragma once

class GraphicsSocket;

namespace nodeeditor {

/**
 * C++ analogue of node_edge_validators + Edge::validateEdge.
 * Validates an undirected pair (socket A, socket B) where A is typically
 * the drag origin and B the drop target, matching Python validateEdge(start, end).
 */
class EdgeValidators {
public:
    static void registerDefaults();

    [[nodiscard]] static bool validate(GraphicsSocket* a, GraphicsSocket* b);

private:
    static bool twoOutputsOrTwoInputs(GraphicsSocket* a, GraphicsSocket* b);
    static bool sameNode(GraphicsSocket* a, GraphicsSocket* b);
    static bool sameSocketType(GraphicsSocket* a, GraphicsSocket* b);
};

} // namespace nodeeditor
