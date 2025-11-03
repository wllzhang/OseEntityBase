/**
 * @file QGraphicsViewAdapter.h
 * @brief QGraphicsView适配器头文件
 * 
 * 功能：将Qt的QGraphicsView嵌入到OSG场景中
 * 主要功能：
 *   - 在OSG纹理上渲染Qt widget
 *   - 双向事件转发（Qt<->OSG）
 *   - 三缓冲图像管理
 */

#pragma once

#ifndef QGRAPHICSVIEWADAPTER_H
#define QGRAPHICSVIEWADAPTER_H

#include <QGLWidget>

#include <osg/Image>
#include <osg/observer_ptr>

#include <QPointer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QApplication>
#include <QPainter>
#include <QtEvents>

namespace osgQt
{

// 获取或创建QApplication实例
extern QCoreApplication* getOrCreateQApplication();

/**
 * @brief QGraphicsViewAdapter类：QGraphicsView适配器
 * 
 * 功能：在OSG场景中嵌入Qt Widget
 */
class QGraphicsViewAdapter : public QObject
{
	Q_OBJECT

public:

	QGraphicsViewAdapter(osg::Image* image, QWidget* widget);
	~QGraphicsViewAdapter(void){};

	void setUpKeyMap();

	bool sendPointerEvent(int x, int y, int buttonMask);

	bool sendKeyEvent(int key, bool keyDown);

	void setFrameLastRendered(const osg::FrameStamp* frameStamp);

	void clearWriteBuffer();

	bool requiresRendering() const { return _requiresRendering; }

	void render();

	void assignImage(unsigned int i);

	void resize(int width, int height);

	void setBackgroundColor(QColor color) { _backgroundColor = color; }
	QColor getBackgroundColor() const     { return _backgroundColor; }

	/** The 'background widget' will ignore mouse/keyboard events and let following handlers handle them
        It is mainly used for integrating scene graph and full-screen UIs
    */
    void setBackgroundWidget(QWidget* w) { _backgroundWidget = w; }
    QWidget* getBackgroundWidget() { return _backgroundWidget; }

    QGraphicsView* getQGraphicsView() { return _graphicsView; }
    QGraphicsScene* getQGraphicsScene() { return _graphicsScene; }

protected:

	bool handlePointerEvent(int x, int y, int buttonMask);
	bool handleKeyEvent(int key, bool keyDown);
	QWidget* getWidgetAt(const QPoint& pos);

	osg::observer_ptr<osg::Image>   _image;
	QWidget*                        _backgroundWidget;

	int                             _previousButtonMask;
	int                             _previousMouseX;
	int                             _previousMouseY;
	int                             _previousQtMouseX;
	int                             _previousQtMouseY;
	bool                            _previousSentEvent;
	bool                            _requiresRendering;

	int _width;
	int _height;

	typedef std::map<int, Qt::Key> KeyMap;
	KeyMap                          _keyMap;
	Qt::KeyboardModifiers           _qtKeyModifiers;

	QColor                          _backgroundColor;
	QPointer<QGraphicsView>         _graphicsView;
	QPointer<QGraphicsScene>        _graphicsScene;
	QPointer<QWidget>               _widget;

	OpenThreads::Mutex              _qimagesMutex;
	OpenThreads::Mutex              _qresizeMutex;
	unsigned int                    _previousFrameNumber;
	bool                            _newImageAvailable;
	unsigned int                    _currentRead;
	unsigned int                    _currentWrite;
	unsigned int                    _previousWrite;
	QImage                          _qimages[3];

	virtual void customEvent ( QEvent * event ) ;

	private slots:

		void repaintRequestedSlot(const QList<QRectF> &regions);
		void repaintRequestedSlot(const QRectF &region);

};

}

#endif