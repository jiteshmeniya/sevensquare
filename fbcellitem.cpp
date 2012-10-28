#include <QDebug>
#include <QPainter>
#include <QMutexLocker>
#include <QDateTime>
#include <QImage>

#include <stdint.h>

#include "fbcellitem.h"
#include "debug.h"

FBCellItem::FBCellItem(QGraphicsItem *parent)
{
	fbConnected = false;
}

FBCellItem::FBCellItem(const QPixmap &pixmap) :
        pixmap(pixmap),
	fbConnected(false)
{
	cellSize.setWidth(pixmap.width());
	cellSize.setHeight(pixmap.height());

	fbSize = cellSize;
	fb = new QPixmap(fbSize);
	fb->fill(QColor(Qt::black));

	lastSum = -1;
}

QRectF FBCellItem::boundingRect() const
{
    return QRectF(0, 0, cellSize.width(), cellSize.height());
}

void FBCellItem::setFBSize(QSize size)
{
	QMutexLocker locker(&mutex);

	if (fbSize == size)
		return;

	qDebug() << "New FB size:" << size << fbSize;
	fbSize = size;

	if (fb != NULL)
		free(fb);

	fb = new QPixmap(fbSize);
	fb->fill(QColor(Qt::black));
}

int FBCellItem::setFBRaw(QByteArray *raw)
{
	QMutexLocker locker(&mutex);
	quint16 sum;
       
	// TODO: Check and update partially, block by block
	sum = qChecksum(raw->data(), raw->length());
	if (sum == lastSum)
		return UPDATE_IGNORED;

	lastSum = sum;
	bytes = *raw;
	update(boundingRect());

	return UPDATE_DONE;
}

void FBCellItem::paintFB(QPainter *painter)
{
	QMutexLocker locker(&mutex);
	QPainter fbPainter;
	QImage image;
	uchar *buf, *p, *n;
	int x, y;

	if (bytes.length() < fbSize.width() * fbSize.height() * 4)
	{
		qDebug() << "Invalid data:" << bytes.length();
		return;
	}
	DT_TRACE("PAIT RAW S");

	buf = (uchar*) bytes.data();
	p = n = buf;

	// RGBX32 -> RGB888
	for (y = 0; y < fbSize.height(); y++) {
		for (x = 0; x < fbSize.width(); x++) {
			*p++ = *n++;
			*p++ = *n++;
			*p++ = *n++;
			n++;
		}
	}

	fbPainter.begin(fb);
	image = QImage((const uchar*)buf, fbSize.width(), fbSize.height(),
			QImage::Format_RGB888);
	fbPainter.drawImage(fb->rect(), image);
	fbPainter.end();

	DT_TRACE("PAIT RAW E");
}

void FBCellItem::paint(QPainter *painter,
                         const QStyleOptionGraphicsItem *option,
                         QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    DT_TRACE("FB PAINT S");
    if (fbConnected) {
	paintFB(painter);
	pixmap = fb->scaled(cellSize,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
    }

    if (pixmap.isNull()) {
        return;
    }

    painter->drawPixmap(pixmap.rect(), pixmap);
    DT_TRACE("FB PAINT E");
}

void FBCellItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item pressed: " << curr_pos;
}

void FBCellItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item moveded: " << curr_pos;
}

void FBCellItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item clicked, curr pos: " << curr_pos
    //        << ", orig pos: " << orig_pos;
}
