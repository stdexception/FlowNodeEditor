#include "EdgeDragging.hpp"
#include "EditorScene.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsSocket.hpp"
#include "GraphicsView.hpp"
#include "SceneHistory.hpp"
#include "nodeeditor/EdgeValidators.hpp"

EdgeDragging::EdgeDragging(GraphicsView* view)
    : view_(view)
{
}

void EdgeDragging::edgeDragStart(GraphicsSocket* itemSocket)
{
    cancelDrag();
    if (!view_ || !itemSocket || !itemSocket->node() || !itemSocket->node()->document())
        return;
    dragStart_ = itemSocket;
    EditorScene* doc = itemSocket->node()->document();
    dragEdge_ = new GraphicsEdge(doc, dragStart_, nullptr);
    dragCursorScene_ = dragStart_->scenePos();
    dragEdge_->setFreeEndScene(dragCursorScene_);
}

bool EdgeDragging::edgeDragEnd(GraphicsSocket* releaseSocket)
{
    if (!dragEdge_ || !dragStart_) {
        cancelDrag();
        return false;
    }

    EditorScene* doc = dragEdge_->document();
    if (!doc) {
        cancelDrag();
        return false;
    }

    // Python: early out if not released on a socket — cancel temporary edge
    if (!releaseSocket) {
        dragEdge_->removeFromDocument();
        delete dragEdge_;
        dragEdge_ = nullptr;
        dragStart_ = nullptr;
        return false;
    }

    if (!nodeeditor::EdgeValidators::validate(dragStart_, releaseSocket)) {
        dragEdge_->removeFromDocument();
        delete dragEdge_;
        dragEdge_ = nullptr;
        dragStart_ = nullptr;
        return false;
    }

    GraphicsSocket* outSock = nullptr;
    GraphicsSocket* inSock = nullptr;
    if (dragStart_->isOutput()) {
        outSock = dragStart_;
        inSock = releaseSocket;
    } else if (releaseSocket->isOutput()) {
        outSock = releaseSocket;
        inSock = dragStart_;
    } else {
        dragEdge_->removeFromDocument();
        delete dragEdge_;
        dragEdge_ = nullptr;
        dragStart_ = nullptr;
        return false;
    }

    if (inSock == outSock) {
        dragEdge_->removeFromDocument();
        delete dragEdge_;
        dragEdge_ = nullptr;
        dragStart_ = nullptr;
        return false;
    }

    // Remove temporary drag edge from document (without destructor removing from wrong state)
    dragEdge_->removeFromDocument();
    delete dragEdge_;
    dragEdge_ = nullptr;
    dragStart_ = nullptr;

    if (!inSock->multiEdges())
        inSock->removeAllEdges();
    if (!outSock->multiEdges())
        outSock->removeAllEdges();

    new GraphicsEdge(doc, outSock, inSock);
    if (doc->history())
        doc->history()->storeHistory(QStringLiteral("Created edge"), true);
    return true;
}

void EdgeDragging::updateDestination(qreal x, qreal y)
{
    dragCursorScene_ = QPointF(x, y);
    if (dragEdge_)
        dragEdge_->setFreeEndScene(dragCursorScene_);
}

void EdgeDragging::cancelDrag()
{
    if (dragEdge_) {
        dragEdge_->removeFromDocument();
        delete dragEdge_;
        dragEdge_ = nullptr;
    }
    dragStart_ = nullptr;
}
