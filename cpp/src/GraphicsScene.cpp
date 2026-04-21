#include "GraphicsScene.hpp"
#include "EditorScene.hpp"

#include <cmath>
#include <QPainter>

GraphicsScene::GraphicsScene(EditorScene* document, QObject* parent)
    : QGraphicsScene(parent)
    , document_(document)
    , colorBackground_(QStringLiteral("#202b3c"))
    , colorLight_(QStringLiteral("#2a3343"))
    , colorDark_(QStringLiteral("#2a3343"))
{
    setItemIndexMethod(QGraphicsScene::NoIndex);
    setBackgroundBrush(colorBackground_);

    penLight_.setColor(colorLight_);
    penDark_.setColor(colorDark_);
    penLight_.setWidthF(0.3);
    penDark_.setWidthF(0.8);
}

void GraphicsScene::setGridSceneSize(int width, int height)
{
    setSceneRect(-width / 2, -height / 2, width, height);
}

void GraphicsScene::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsScene::drawBackground(painter, rect);

    const qreal left = std::floor(rect.left());
    const qreal right = std::ceil(rect.right());
    const qreal top = std::floor(rect.top());
    const qreal bottom = std::ceil(rect.bottom());

    const int ileft = static_cast<int>(left);
    const int iright = static_cast<int>(right);
    const int itop = static_cast<int>(top);
    const int ibottom = static_cast<int>(bottom);

    int firstLeft = ileft - (ileft % gridSize_);
    int firstTop = itop - (itop % gridSize_);
    if (firstLeft > ileft)
        firstLeft -= gridSize_;
    if (firstTop > itop)
        firstTop -= gridSize_;

    const int major = gridSize_ * gridSquares_;

    QVector<QLineF> linesLight;
    QVector<QLineF> linesDark;

    for (int x = firstLeft; x <= iright; x += gridSize_) {
        if (x % major != 0)
            linesLight.append(QLineF(x, top, x, bottom));
        else
            linesDark.append(QLineF(x, top, x, bottom));
    }
    for (int y = firstTop; y <= ibottom; y += gridSize_) {
        if (y % major != 0)
            linesLight.append(QLineF(left, y, right, y));
        else
            linesDark.append(QLineF(left, y, right, y));
    }

    painter->setPen(penLight_);
    painter->drawLines(linesLight);
    painter->setPen(penDark_);
    painter->drawLines(linesDark);
}
