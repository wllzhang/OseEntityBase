/**
 * @file GraphicsWindowQt.h
 * @brief OSG图形窗口的Qt实现头文件
 * 
 * 功能：提供基于Qt的OpenGL图形窗口，集成OSG场景渲染到Qt应用程序中
 * 主要类：
 *   - GLWidget: 基于QGLWidget的OpenGL Widget，处理Qt事件并转发到OSG
 *   - GraphicsWindowQt: OSG图形窗口的Qt实现，管理窗口生命周期和渲染
 */

#pragma once

#ifndef OSGVIEWER_GRAPHICSWINDOWQT
#define OSGVIEWER_GRAPHICSWINDOWQT

#include <osgViewer/GraphicsWindow>
//#include <osgQt/Export>

#include <QtCore/QMutex>
#include <QtGui/QtEvents>
#include <QtCore/QQueue>
#include <QtCore/QSet>
#include <QtOpenGL/QGLWidget>
#include <osg/Version>

// 前向声明MapStateManager类和GeoEntityManager类
class MapStateManager;
class GeoEntityManager;

//#include <osgViewer/CompositeViewer>
//#include <QtCore/QMutexLocker>


class QInputEvent;
class QGestureEvent;

namespace osgViewer {
	class ViewerBase;
}

namespace osgQt
{

	// 前向声明
	class GraphicsWindowQt;

	// 初始化Qt窗口系统
	void initQtWindowingSystem();

	// 设置OSG查看器，用于驱动场景渲染
	void setViewer( osgViewer::ViewerBase *viewer );

/**
 * @brief GLWidget类：基于QGLWidget的OpenGL Widget
 * 
 * 功能：
 *   - 将Qt事件转发到OSG事件队列
 *   - 支持多线程环境下的延迟事件处理
 *   - 提供地图状态管理和实体管理接口
 *   - 处理高DPI显示器的设备像素比
 */
class GLWidget : public QGLWidget
{

public:
	/**
	 * @brief 构造函数：使用默认GL格式
	 * @param parent 父widget
	 * @param shareWidget 共享的GL widget
	 * @param f 窗口标志
	 * @param forwardKeyEvents 是否转发键盘事件到Qt
	 */
	GLWidget( QWidget* parent = NULL , 
		const QGLWidget* shareWidget = NULL , 
		Qt::WindowFlags f = 0 , 
		bool forwardKeyEvents = false  );

	/**
	 * @brief 构造函数：使用指定的GL上下文
	 * @param context GL上下文
	 * @param parent 父widget
	 * @param shareWidget 共享的GL widget
	 * @param f 窗口标志
	 * @param forwardKeyEvents 是否转发键盘事件到Qt
	 */
	GLWidget( QGLContext* context, 
		QWidget* parent = NULL, 
		const QGLWidget* shareWidget = NULL, 
		Qt::WindowFlags f = 0, 
		bool forwardKeyEvents = false );

	/**
	 * @brief 构造函数：使用指定的GL格式
	 * @param format GL格式
	 * @param parent 父widget
	 * @param shareWidget 共享的GL widget
	 * @param f 窗口标志
	 * @param forwardKeyEvents 是否转发键盘事件到Qt
	 */
	GLWidget( const QGLFormat& format, 
		QWidget* parent = NULL, 
		const QGLWidget* shareWidget = NULL, 
		Qt::WindowFlags f = 0, 
		bool forwardKeyEvents = false );

	// 析构函数
	virtual ~GLWidget();

	// 设置/获取关联的GraphicsWindowQt指针
	inline void setGraphicsWindow( GraphicsWindowQt* gw ) { _gw = gw; }
	inline GraphicsWindowQt* getGraphicsWindow() { return _gw; }
	inline const GraphicsWindowQt* getGraphicsWindow() const { return _gw; }

	// 获取/设置是否转发键盘事件
	inline bool getForwardKeyEvents() const { return _forwardKeyEvents; }
	virtual void setForwardKeyEvents( bool f ) { _forwardKeyEvents = f; }

	// 获取/设置触摸事件是否启用
	inline bool getTouchEventsEnabled() const { return _touchEventsEnabled; }
	void setTouchEventsEnabled( bool e );

	// ===== 地图状态管理器接口 =====
	void setMapStateManager(MapStateManager* manager);
	
	// ===== 实体管理器接口 =====
	void setEntityManager(GeoEntityManager* manager);

	void setKeyboardModifiers( QInputEvent* event );
	virtual void keyPressEvent( QKeyEvent* event );
	virtual void keyReleaseEvent( QKeyEvent* event );
	virtual void mousePressEvent( QMouseEvent* event );
	virtual void mouseReleaseEvent( QMouseEvent* event );
	virtual void mouseDoubleClickEvent( QMouseEvent* event );
	virtual void mouseMoveEvent( QMouseEvent* event );
	virtual void wheelEvent( QWheelEvent* event );
	virtual bool gestureEvent( QGestureEvent* event );

protected:
	int getNumDeferredEvents()
	{
		QMutexLocker lock(&_deferredEventQueueMutex);
		return _deferredEventQueue.count();
	}

	void enqueueDeferredEvent(QEvent::Type eventType, QEvent::Type removeEventType = QEvent::None)
	{
		QMutexLocker lock(&_deferredEventQueueMutex);

		if (removeEventType != QEvent::None)
		{
			if (_deferredEventQueue.removeOne(removeEventType))
				_eventCompressor.remove(eventType);
		}

		if (_eventCompressor.find(eventType) == _eventCompressor.end())
		{
			_deferredEventQueue.enqueue(eventType);
			_eventCompressor.insert(eventType);
		}
	}

	void processDeferredEvents();

	// friend class：友元类
	// 在一个类中指明其他的类（或者）函数能够直接访问该类中的private和protected成员。
	friend class GraphicsWindowQt;	
	GraphicsWindowQt* _gw;
	
	QMutex _deferredEventQueueMutex;
	QQueue<QEvent::Type> _deferredEventQueue;
	QSet<QEvent::Type> _eventCompressor;

	bool _touchEventsEnabled;

	bool _forwardKeyEvents;
	qreal _devicePixelRatio;

	// ===== 地图状态管理器支持 =====
	MapStateManager* _mapStateManager;  // 地图状态管理器指针
	
	// ===== 实体管理器支持 =====
	GeoEntityManager* _entityManager;  // 实体管理器指针

	virtual void resizeEvent( QResizeEvent* event );
	virtual void moveEvent( QMoveEvent* event );
	virtual void glDraw();
	virtual bool event( QEvent* event );

};



/**
 * @brief GraphicsWindowQt类：OSG图形窗口的Qt实现
 * 
 * 功能：
 *   - 实现osgViewer::GraphicsWindow接口
 *   - 管理GLWidget的生命周期
 *   - 处理窗口的创建、销毁、大小调整等操作
 *   - 实现OpenGL上下文的创建和管理
 */
class GraphicsWindowQt: public osgViewer::GraphicsWindow
{
public:
	/**
	 * @brief 构造函数：从Traits创建
	 * @param traits 图形上下文特性
	 * @param parent 父widget
	 * @param shareWidget 共享的GL widget
	 * @param f 窗口标志
	 */
	GraphicsWindowQt( osg::GraphicsContext::Traits* traits, QWidget* parent = NULL, const QGLWidget* shareWidget = NULL, Qt::WindowFlags f = 0 );
	
	/**
	 * @brief 构造函数：从已有GLWidget创建
	 * @param widget GL widget
	 */
	GraphicsWindowQt( GLWidget* widget );
	
	// 析构函数
	virtual ~GraphicsWindowQt();

	// 获取GLWidget指针
	inline GLWidget* getGLWidget() { return _widget; }
	inline const GLWidget* getGLWidget() const { return _widget; }

	/// @deprecated 使用getGLWidget()代替
	inline GLWidget* getGraphWidget() { return _widget; }
	/// @deprecated 使用getGLWidget()代替
	inline const GLWidget* getGraphWidget() const { return _widget; }

	/**
	 * @brief WindowData结构体：用于传递窗口数据
	 */
	struct WindowData : public osg::Referenced
	{
		WindowData( GLWidget* widget = NULL, QWidget* parent = NULL ): _widget(widget), _parent(parent) {}
		GLWidget* _widget;  // GL widget指针
		QWidget* _parent;   // 父widget指针
	};


	bool init( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f );

	static QGLFormat traits2qglFormat( const osg::GraphicsContext::Traits* traits );
	static void qglFormat2traits( const QGLFormat& format, osg::GraphicsContext::Traits* traits );
	static osg::GraphicsContext::Traits* createTraits( const QGLWidget* widget );

	virtual bool setWindowRectangleImplementation( int x, int y, int width, int height );
	virtual void getWindowRectangle( int& x, int& y, int& width, int& height );
	virtual bool setWindowDecorationImplementation( bool windowDecoration );
	virtual bool getWindowDecoration() const;
	virtual void grabFocus();
	virtual void grabFocusIfPointerInWindow();
	virtual void raiseWindow();
	virtual void setWindowName( const std::string& name );
	virtual std::string getWindowName();
	virtual void useCursor( bool cursorOn );
	virtual void setCursor( MouseCursor cursor );
	inline bool getTouchEventsEnabled() const { return _widget->getTouchEventsEnabled(); }
	virtual void setTouchEventsEnabled( bool e ) { _widget->setTouchEventsEnabled(e); }


	virtual bool valid() const;
	virtual bool realizeImplementation();
	virtual bool isRealizedImplementation() const;
	virtual void closeImplementation();
	virtual bool makeCurrentImplementation();
	virtual bool releaseContextImplementation();
	virtual void swapBuffersImplementation();
	virtual void runOperations();

	virtual void requestWarpPointer( float x, float y );

protected:

	friend class GLWidget;
	GLWidget* _widget;
	bool _ownsWidget;
	QCursor _currentCursor;
	bool _realized;

};

}


#endif