/**
 * @file GraphicsWindowQt.cpp
 * @brief OSG图形窗口的Qt实现源文件
 * 
 * 实现GraphicsWindowQt类和GLWidget类，以及相关的内部工具类
 */

#include "GraphicsWindowQt.h"

#include <osg/DeleteHandler>
//#include <osgQt/GraphicsWindowQt>
#include <osgViewer/ViewerBase>
#include <QtGui/QInputEvent>
#include <QtCore/QPointer>

// 包含MapStateManager和GeoEntityManager的完整定义
#include "geo/mapstatemanager.h"
#include "geo/geoentitymanager.h"

using namespace osgQt;

// @cond INTERNAL
// 以下类和结构是内部实现，不在公开文档中

// ============================================================================
// QtKeyboardMap类：Qt键盘按键映射到OSG按键（内部实现）
// ============================================================================
// 功能：将Qt的键盘事件转换为OSG的键盘事件
// 用途：在Qt和OSG之间建立键盘事件映射关系
// 注意：这是内部工具类，不对外暴露
// ============================================================================

class QtKeyboardMap
{

public:
	// 构造函数：初始化Qt键盘按键到OSG按键的映射表
	QtKeyboardMap()
	{
		mKeyMap[Qt::Key_Escape     ] = osgGA::GUIEventAdapter::KEY_Escape;
		mKeyMap[Qt::Key_Delete   ] = osgGA::GUIEventAdapter::KEY_Delete;
		mKeyMap[Qt::Key_Home       ] = osgGA::GUIEventAdapter::KEY_Home;
		mKeyMap[Qt::Key_Enter      ] = osgGA::GUIEventAdapter::KEY_KP_Enter;
		mKeyMap[Qt::Key_End        ] = osgGA::GUIEventAdapter::KEY_End;
		mKeyMap[Qt::Key_Return     ] = osgGA::GUIEventAdapter::KEY_Return;
		mKeyMap[Qt::Key_PageUp     ] = osgGA::GUIEventAdapter::KEY_Page_Up;
		mKeyMap[Qt::Key_PageDown   ] = osgGA::GUIEventAdapter::KEY_Page_Down;
		mKeyMap[Qt::Key_Left       ] = osgGA::GUIEventAdapter::KEY_Left;
		mKeyMap[Qt::Key_Right      ] = osgGA::GUIEventAdapter::KEY_Right;
		mKeyMap[Qt::Key_Up         ] = osgGA::GUIEventAdapter::KEY_Up;
		mKeyMap[Qt::Key_Down       ] = osgGA::GUIEventAdapter::KEY_Down;
		mKeyMap[Qt::Key_Backspace  ] = osgGA::GUIEventAdapter::KEY_BackSpace;
		mKeyMap[Qt::Key_Tab        ] = osgGA::GUIEventAdapter::KEY_Tab;
		mKeyMap[Qt::Key_Space      ] = osgGA::GUIEventAdapter::KEY_Space;
		mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
		mKeyMap[Qt::Key_Alt      ] = osgGA::GUIEventAdapter::KEY_Alt_L;
		mKeyMap[Qt::Key_Shift    ] = osgGA::GUIEventAdapter::KEY_Shift_L;
		mKeyMap[Qt::Key_Control  ] = osgGA::GUIEventAdapter::KEY_Control_L;
		mKeyMap[Qt::Key_Meta     ] = osgGA::GUIEventAdapter::KEY_Meta_L;

		mKeyMap[Qt::Key_F1             ] = osgGA::GUIEventAdapter::KEY_F1;
		mKeyMap[Qt::Key_F2             ] = osgGA::GUIEventAdapter::KEY_F2;
		mKeyMap[Qt::Key_F3             ] = osgGA::GUIEventAdapter::KEY_F3;
		mKeyMap[Qt::Key_F4             ] = osgGA::GUIEventAdapter::KEY_F4;
		mKeyMap[Qt::Key_F5             ] = osgGA::GUIEventAdapter::KEY_F5;
		mKeyMap[Qt::Key_F6             ] = osgGA::GUIEventAdapter::KEY_F6;
		mKeyMap[Qt::Key_F7             ] = osgGA::GUIEventAdapter::KEY_F7;
		mKeyMap[Qt::Key_F8             ] = osgGA::GUIEventAdapter::KEY_F8;
		mKeyMap[Qt::Key_F9             ] = osgGA::GUIEventAdapter::KEY_F9;
		mKeyMap[Qt::Key_F10            ] = osgGA::GUIEventAdapter::KEY_F10;
		mKeyMap[Qt::Key_F11            ] = osgGA::GUIEventAdapter::KEY_F11;
		mKeyMap[Qt::Key_F12            ] = osgGA::GUIEventAdapter::KEY_F12;
		mKeyMap[Qt::Key_F13            ] = osgGA::GUIEventAdapter::KEY_F13;
		mKeyMap[Qt::Key_F14            ] = osgGA::GUIEventAdapter::KEY_F14;
		mKeyMap[Qt::Key_F15            ] = osgGA::GUIEventAdapter::KEY_F15;
		mKeyMap[Qt::Key_F16            ] = osgGA::GUIEventAdapter::KEY_F16;
		mKeyMap[Qt::Key_F17            ] = osgGA::GUIEventAdapter::KEY_F17;
		mKeyMap[Qt::Key_F18            ] = osgGA::GUIEventAdapter::KEY_F18;
		mKeyMap[Qt::Key_F19            ] = osgGA::GUIEventAdapter::KEY_F19;
		mKeyMap[Qt::Key_F20            ] = osgGA::GUIEventAdapter::KEY_F20;

		mKeyMap[Qt::Key_hyphen         ] = '-';
		mKeyMap[Qt::Key_Equal         ] = '=';

		mKeyMap[Qt::Key_division      ] = osgGA::GUIEventAdapter::KEY_KP_Divide;
		mKeyMap[Qt::Key_multiply      ] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
		mKeyMap[Qt::Key_Minus         ] = '-';
		mKeyMap[Qt::Key_Plus          ] = '+';
		//mKeyMap[Qt::Key_H              ] = osgGA::GUIEventAdapter::KEY_KP_Home;
		//mKeyMap[Qt::Key_                    ] = osgGA::GUIEventAdapter::KEY_KP_Up;
		//mKeyMap[92                    ] = osgGA::GUIEventAdapter::KEY_KP_Page_Up;
		//mKeyMap[86                    ] = osgGA::GUIEventAdapter::KEY_KP_Left;
		//mKeyMap[87                    ] = osgGA::GUIEventAdapter::KEY_KP_Begin;
		//mKeyMap[88                    ] = osgGA::GUIEventAdapter::KEY_KP_Right;
		//mKeyMap[83                    ] = osgGA::GUIEventAdapter::KEY_KP_End;
		//mKeyMap[84                    ] = osgGA::GUIEventAdapter::KEY_KP_Down;
		//mKeyMap[85                    ] = osgGA::GUIEventAdapter::KEY_KP_Page_Down;
		mKeyMap[Qt::Key_Insert        ] = osgGA::GUIEventAdapter::KEY_KP_Insert;
		//mKeyMap[Qt::Key_Delete        ] = osgGA::GUIEventAdapter::KEY_KP_Delete;
	}

	// 析构函数
	~QtKeyboardMap()
	{
	}

	/**
	 * @brief 将Qt键盘事件映射到OSG键盘码
	 * @param event Qt键盘事件
	 * @return OSG键盘码
	 */
	int remapKey(QKeyEvent* event)
	{
		// 在映射表中查找Qt按键
		KeyMap::iterator itr = mKeyMap.find(event->key());
		if (itr == mKeyMap.end())
		{
			// 如果找不到映射，直接返回按键的ASCII码
			return int(*(event->text().toLatin1().data()));
		}
		else
			return itr->second;
	}

private:
	typedef std::map<unsigned int, int> KeyMap;  // Qt按键码到OSG按键码的映射表
	KeyMap mKeyMap;  // 存储映射关系

};

// 全局静态实例，用于键盘映射
static QtKeyboardMap s_QtKeyboardMap;

// ============================================================================
// HeartBeat类：场景重渲染定时器（内部实现）
// ============================================================================
// 功能：负责在Qt事件循环中驱动OSG场景的渲染
// 用途：将OSG的渲染循环集成到Qt的事件循环中
// ============================================================================
/// The object responsible for the scene re-rendering.
class HeartBeat : public QObject
{
public:
	int _timerId;  // Qt定时器ID
	osg::Timer _lastFrameStartTime;  // 上一帧开始时间
	osg::observer_ptr< osgViewer::ViewerBase > _viewer;  // OSG查看器指针

	virtual ~HeartBeat();

	// 初始化心跳定时器
	void init( osgViewer::ViewerBase *viewer );
	// 停止定时器
	void stopTimer();
	// 定时器事件处理函数
	void timerEvent( QTimerEvent *event );

	// 获取单例实例
	static HeartBeat* instance();

private:
	HeartBeat();

	static QPointer<HeartBeat> heartBeat;  // 单例指针
};


QPointer<HeartBeat> HeartBeat::heartBeat;

// 设备像素比宏：用于高DPI显示器支持
#if (QT_VERSION < QT_VERSION_CHECK(5, 2, 0))
#define GETDEVICEPIXELRATIO() 1.0
#else
#define GETDEVICEPIXELRATIO() devicePixelRatio()
#endif

// ============================================================================
// GLWidget类实现：基于QGLWidget的OpenGL Widget
// ============================================================================

// 构造函数1：使用默认GL格式
GLWidget::GLWidget( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f, bool forwardKeyEvents )
	: QGLWidget(parent, shareWidget, f),
	_gw( NULL ),
	_touchEventsEnabled( false ),
	_forwardKeyEvents( forwardKeyEvents ),
	_mapStateManager( NULL ),  // 初始化地图状态管理器指针为空
	_entityManager( NULL )     // 初始化实体管理器指针为空
{
	_devicePixelRatio = GETDEVICEPIXELRATIO();
}

// 构造函数2：使用指定的GL上下文
GLWidget::GLWidget( QGLContext* context, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f,
	bool forwardKeyEvents )
	: QGLWidget(context, parent, shareWidget, f),
	_gw( NULL ),
	_touchEventsEnabled( false ),
	_forwardKeyEvents( forwardKeyEvents ),
	_mapStateManager( NULL ),  // 初始化地图状态管理器指针为空
	_entityManager( NULL )     // 初始化实体管理器指针为空
{
	_devicePixelRatio = GETDEVICEPIXELRATIO();
}

// 构造函数3：使用指定的GL格式
GLWidget::GLWidget( const QGLFormat& format, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f,
	bool forwardKeyEvents )
	: QGLWidget(format, parent, shareWidget, f),
	_gw( NULL ),
	_touchEventsEnabled( false ),
	_forwardKeyEvents( forwardKeyEvents ),
	_mapStateManager( NULL ),  // 初始化地图状态管理器指针为空
	_entityManager( NULL )     // 初始化实体管理器指针为空
{
	_devicePixelRatio = GETDEVICEPIXELRATIO();
}

// 析构函数
GLWidget::~GLWidget()
{
	// 关闭GraphicsWindowQt并移除引用
	if( _gw )
	{
		_gw->close();
		_gw->_widget = NULL;
		_gw = NULL;
	}
}

/**
 * @brief 启用或禁用触摸事件（如缩放手势）
 * @param e 是否启用
 */
void GLWidget::setTouchEventsEnabled(bool e)
{
#ifdef USE_GESTURES
	if (e==_touchEventsEnabled)
		return;

	_touchEventsEnabled = e;

	if (_touchEventsEnabled)
	{
		// 启用手势识别（如捏合缩放）
		grabGesture(Qt::PinchGesture);
	}
	else
	{
		// 禁用手势识别
		ungrabGesture(Qt::PinchGesture);
	}
#endif
}

// ===== 地图状态管理器相关方法 =====
void GLWidget::setMapStateManager(MapStateManager* manager)
{
	// 设置地图状态管理器指针
	// 这个方法允许外部设置地图状态管理器，用于监听鼠标事件
	_mapStateManager = manager;
}

void GLWidget::setEntityManager(GeoEntityManager* manager)
{
	// 设置实体管理器指针
	// 这个方法允许外部设置实体管理器，用于处理实体选择等事件
	_entityManager = manager;
}

/**
 * @brief 处理延迟事件队列
 * 用于在多线程环境中安全处理某些需要在主线程中执行的事件
 */
void GLWidget::processDeferredEvents()
{
	QQueue<QEvent::Type> deferredEventQueueCopy;
	{
		// 加锁复制事件队列
		QMutexLocker lock(&_deferredEventQueueMutex);
		deferredEventQueueCopy = _deferredEventQueue;
		_eventCompressor.clear();
		_deferredEventQueue.clear();
	}

	// 处理所有延迟的事件
	while (!deferredEventQueueCopy.isEmpty())
	{
		QEvent event(deferredEventQueueCopy.dequeue());
		QGLWidget::event(&event);
	}
}


/**
 * @brief 事件处理函数
 * @param event Qt事件
 * @return 是否处理了该事件
 * 
 * 说明：
 * QEvent::Hide - 处理Qt在隐藏widget前调用glFinish的workaround
 *                在OSG多线程环境中，context可能在另一个线程中活动
 * QEvent::ParentChange - 处理重新父级化时可能创建的新的GL context
 *                        这些事件需要在正确的线程中延迟执行
 */
bool GLWidget::event( QEvent* event )
{
#ifdef USE_GESTURES
	if ( event->type()==QEvent::Gesture )
		return gestureEvent(static_cast<QGestureEvent*>(event));
#endif

	// QEvent::Hide
	//
	// workaround "Qt-workaround" that does glFinish before hiding the widget
	// (the Qt workaround was seen at least in Qt 4.6.3 and 4.7.0)
	//
	// Qt makes the context current, performs glFinish, and releases the context.
	// This makes the problem in OSG multithreaded environment as the context
	// is active in another thread, thus it can not be made current for the purpose
	// of glFinish in this thread.

	// QEvent::ParentChange
	//
	// Reparenting GLWidget may create a new underlying window and a new GL context.
	// Qt will then call doneCurrent on the GL context about to be deleted. The thread
	// where old GL context was current has no longer current context to render to and
	// we cannot make new GL context current in this thread.

	// We workaround above problems by deferring execution of problematic event requests.
	// These events has to be enqueue and executed later in a main GUI thread (GUI operations
	// outside the main thread are not allowed) just before makeCurrent is called from the
	// right thread. The good place for doing that is right after swap in a swapBuffersImplementation.

	if (event->type() == QEvent::Hide)
	{
		// enqueue only the last of QEvent::Hide and QEvent::Show
		enqueueDeferredEvent(QEvent::Hide, QEvent::Show);
		return true;
	}
	else if (event->type() == QEvent::Show)
	{
		// enqueue only the last of QEvent::Show or QEvent::Hide
		enqueueDeferredEvent(QEvent::Show, QEvent::Hide);
		return true;
	}
	else if (event->type() == QEvent::ParentChange)
	{
		// enqueue only the last QEvent::ParentChange
		enqueueDeferredEvent(QEvent::ParentChange);
		return true;
	}

	// perform regular event handling
	return QGLWidget::event( event );
}

/**
 * @brief 设置键盘修饰键状态（Shift、Ctrl、Alt等）
 * @param event Qt输入事件
 */
void GLWidget::setKeyboardModifiers( QInputEvent* event )
{
	// 提取修饰键状态
	int modkey = event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier);
	unsigned int mask = 0;
	if ( modkey & Qt::ShiftModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;
	if ( modkey & Qt::ControlModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;
	if ( modkey & Qt::AltModifier ) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;
	// 设置到OSG事件队列
	_gw->getEventQueue()->getCurrentEventState()->setModKeyMask( mask );
}

/**
 * @brief 窗口大小改变事件处理
 * @param event 大小改变事件
 */
void GLWidget::resizeEvent( QResizeEvent* event )
{
	if (_gw == NULL || !_gw->valid())
		return;    
	const QSize& size = event->size();
	int scaled_width = static_cast<int>(size.width()*_devicePixelRatio);
	int scaled_height = static_cast<int>(size.height()*_devicePixelRatio);
	_gw->resized( x(), y(), scaled_width,  scaled_height);
	_gw->getEventQueue()->windowResize( x(), y(), scaled_width, scaled_height );
	_gw->requestRedraw();
}

/**
 * @brief 窗口移动事件处理
 * @param event 移动事件
 */
void GLWidget::moveEvent( QMoveEvent* event )
{
	if (_gw == NULL || !_gw->valid())
		return; 
	const QPoint& pos = event->pos();
	// 考虑高DPI显示器的设备像素比
	int scaled_width = static_cast<int>(width()*_devicePixelRatio);
	int scaled_height = static_cast<int>(height()*_devicePixelRatio);
	_gw->resized( pos.x(), pos.y(), scaled_width,  scaled_height );
	_gw->getEventQueue()->windowResize( pos.x(), pos.y(), scaled_width,  scaled_height );
}

/**
 * @brief 请求重绘
 */
void GLWidget::glDraw()
{
	_gw->requestRedraw();
}


/**
 * @brief 键盘按下事件处理
 * @param event 键盘事件
 */
void GLWidget::keyPressEvent( QKeyEvent* event )
{
	// 设置修饰键状态
	setKeyboardModifiers( event );
	// 映射Qt按键到OSG按键
	int value = s_QtKeyboardMap.remapKey( event );
	_gw->getEventQueue()->keyPress( value );

	// 如果需要转发事件给Qt常规键盘事件处理
	// 例如：按ESC关闭弹出窗口，或转发事件给父widget
	if( _forwardKeyEvents )
		keyPressEvent( event );
}

/**
 * @brief 键盘释放事件处理
 * @param event 键盘事件
 */
void GLWidget::keyReleaseEvent( QKeyEvent* event )
{
	// 忽略自动重复的按键事件
	if( event->isAutoRepeat() )
	{
		event->ignore();
	}
	else
	{
		// 设置修饰键状态并发送释放事件到OSG
		setKeyboardModifiers( event );
		int value = s_QtKeyboardMap.remapKey( event );
		_gw->getEventQueue()->keyRelease( value );
	}

	// 如果需要转发事件给Qt常规键盘事件处理
	if( _forwardKeyEvents )
		keyReleaseEvent( event );
}


void GLWidget::mousePressEvent( QMouseEvent* event )
{
	// ===== OSG鼠标事件处理 =====
	// 将Qt鼠标按钮映射到OSG按钮编号
	int button = 0;
	switch ( event->button() )
	{
	case Qt::LeftButton: button = 1; break;    // 左键 = 1
	case Qt::MidButton: button = 2; break;     // 中键 = 2  
	case Qt::RightButton: button = 3; break;   // 右键 = 3
	case Qt::NoButton: button = 0; break;      // 无按钮 = 0
	default: button = 0; break;
	}
	
	// 设置键盘修饰键状态 (Shift、Ctrl、Alt等)
	setKeyboardModifiers( event );
	
	// 将鼠标按下事件发送到OSG事件队列
	// 注意：坐标需要乘以设备像素比以处理高DPI显示器
	_gw->getEventQueue()->mouseButtonPress( event->x()*_devicePixelRatio, event->y()*_devicePixelRatio, button );
	
	// ===== 地图状态管理器通知 =====
	// 如果设置了地图状态管理器，则通知它鼠标按下事件
	// 这样可以让地图状态管理器获取鼠标位置和状态信息
	if (_mapStateManager) {
		_mapStateManager->onMousePress(event);
	}
	
	// ===== 实体管理器通知 =====
	// 如果设置了实体管理器，则通知它鼠标按下事件
	// 这样可以让实体管理器处理实体选择等事件
	if (_entityManager) {
		_entityManager->onMousePress(event);
	}
}

void GLWidget::mouseReleaseEvent( QMouseEvent* event )
{
	// ===== OSG鼠标事件处理 =====
	// 将Qt鼠标按钮映射到OSG按钮编号
	int button = 0;
	switch ( event->button() )
	{
	case Qt::LeftButton: button = 1; break;    // 左键 = 1
	case Qt::MidButton: button = 2; break;     // 中键 = 2  
	case Qt::RightButton: button = 3; break;   // 右键 = 3
	case Qt::NoButton: button = 0; break;      // 无按钮 = 0
	default: button = 0; break;
	}
	
	// 设置键盘修饰键状态 (Shift、Ctrl、Alt等)
	setKeyboardModifiers( event );
	
	// 将鼠标释放事件发送到OSG事件队列
	// 注意：坐标需要乘以设备像素比以处理高DPI显示器
	_gw->getEventQueue()->mouseButtonRelease( event->x()*_devicePixelRatio, event->y()*_devicePixelRatio, button );
	
	// ===== 地图状态管理器通知 =====
	// 如果设置了地图状态管理器，则通知它鼠标释放事件
	// 这样可以让地图状态管理器获取鼠标位置和状态信息
	if (_mapStateManager) {
		_mapStateManager->onMouseRelease(event);
	}
}

/**
 * @brief 鼠标双击事件处理
 * @param event 鼠标事件
 */
void GLWidget::mouseDoubleClickEvent( QMouseEvent* event )
{
	// 将Qt鼠标按钮映射到OSG按钮编号
	int button = 0;
	switch ( event->button() )
	{
	case Qt::LeftButton: button = 1; break;
	case Qt::MidButton: button = 2; break;
	case Qt::RightButton: button = 3; break;
	case Qt::NoButton: button = 0; break;
	default: button = 0; break;
	}
	setKeyboardModifiers( event );
	// 发送双击事件到OSG，考虑设备像素比
	_gw->getEventQueue()->mouseDoubleButtonPress( event->x()*_devicePixelRatio, event->y()*_devicePixelRatio, button );
}

/**
 * @brief 鼠标移动事件处理
 * @param event 鼠标事件
 */
void GLWidget::mouseMoveEvent( QMouseEvent* event )
{
	// ===== OSG鼠标事件处理 =====
	// 设置键盘修饰键状态 (Shift、Ctrl、Alt等)
	setKeyboardModifiers( event );
	
	// 将鼠标移动事件发送到OSG事件队列
	// 注意：坐标需要乘以设备像素比以处理高DPI显示器
	_gw->getEventQueue()->mouseMotion( event->x()*_devicePixelRatio, event->y()*_devicePixelRatio );
	
	// ===== 地图状态管理器通知 =====
	// 如果设置了地图状态管理器，则通知它鼠标移动事件
	// 这样可以让地图状态管理器实时获取鼠标位置和状态信息
	if (_mapStateManager) {
		_mapStateManager->onMouseMove(event);
	}
}

void GLWidget::wheelEvent( QWheelEvent* event )
{
	// ===== OSG鼠标事件处理 =====
	// 设置键盘修饰键状态 (Shift、Ctrl、Alt等)
	setKeyboardModifiers( event );
	
	// 将滚轮事件发送到OSG事件队列
	// 根据滚轮方向判断是垂直滚动还是水平滚动
	// 根据delta值判断滚动方向 (向上/向下 或 向左/向右)
	_gw->getEventQueue()->mouseScroll(
		event->orientation() == Qt::Vertical ?
        (event->delta()>0 ? osgGA::GUIEventAdapter::SCROLL_DOWN : osgGA::GUIEventAdapter::SCROLL_UP) :
		(event->delta()>0 ? osgGA::GUIEventAdapter::SCROLL_LEFT : osgGA::GUIEventAdapter::SCROLL_RIGHT) );
	
	// ===== 地图状态管理器通知 =====
	// 如果设置了地图状态管理器，则通知它滚轮事件
	// 这样可以让地图状态管理器获取缩放等状态信息
	if (_mapStateManager) {
		_mapStateManager->onWheelEvent(event);
	}
}


#ifdef USE_GESTURES
/**
 * @brief 将Qt手势状态转换为OSG触摸阶段
 * @param state Qt手势状态
 * @return OSG触摸阶段
 */
static osgGA::GUIEventAdapter::TouchPhase translateQtGestureState( Qt::GestureState state )
{
	osgGA::GUIEventAdapter::TouchPhase touchPhase;
	switch ( state )
	{
	case Qt::GestureStarted:
		touchPhase = osgGA::GUIEventAdapter::TOUCH_BEGAN;
		break;
	case Qt::GestureUpdated:
		touchPhase = osgGA::GUIEventAdapter::TOUCH_MOVED;
		break;
	case Qt::GestureFinished:
	case Qt::GestureCanceled:
		touchPhase = osgGA::GUIEventAdapter::TOUCH_ENDED;
		break;
	default:
		touchPhase = osgGA::GUIEventAdapter::TOUCH_UNKNOWN;
	};

	return touchPhase;
}
#endif

/**
 * @brief 手势事件处理（如捏合缩放）
 * @param qevent 手势事件
 * @return 是否处理了该事件
 */
bool GLWidget::gestureEvent( QGestureEvent* qevent )
{
#ifndef USE_GESTURES
	return false;
#else

	bool accept = false;

	if ( QPinchGesture* pinch = static_cast<QPinchGesture *>(qevent->gesture(Qt::PinchGesture) ) )
	{
		const QPointF qcenterf = pinch->centerPoint();
		const float angle = pinch->totalRotationAngle();
		const float scale = pinch->totalScaleFactor();

		const QPoint pinchCenterQt = mapFromGlobal(qcenterf.toPoint());
		const osg::Vec2 pinchCenter( pinchCenterQt.x(), pinchCenterQt.y() );

		//We don't have absolute positions of the two touches, only a scale and rotation
		//Hence we create pseudo-coordinates which are reasonable, and centered around the
		//real position
		const float radius = float(width()+height())/4.0f;
		const osg::Vec2 vector( scale*cos(angle)*radius, scale*sin(angle)*radius);
		const osg::Vec2 p0 = pinchCenter+vector;
		const osg::Vec2 p1 = pinchCenter-vector;

		osg::ref_ptr<osgGA::GUIEventAdapter> event = 0;
		const osgGA::GUIEventAdapter::TouchPhase touchPhase = translateQtGestureState( pinch->state() );
		if ( touchPhase==osgGA::GUIEventAdapter::TOUCH_BEGAN )
		{
			event = _gw->getEventQueue()->touchBegan(0 , touchPhase, p0[0], p0[1] );
		}
		else if ( touchPhase==osgGA::GUIEventAdapter::TOUCH_MOVED )
		{
			event = _gw->getEventQueue()->touchMoved( 0, touchPhase, p0[0], p0[1] );
		}
		else
		{
			event = _gw->getEventQueue()->touchEnded( 0, touchPhase, p0[0], p0[1], 1 );
		}

		if ( event )
		{
			event->addTouchPoint( 1, touchPhase, p1[0], p1[1] );
			accept = true;
		}
	}

	if ( accept )
		qevent->accept();

	return accept;
#endif
}



// ============================================================================
// GraphicsWindowQt类实现：OSG图形窗口的Qt实现
// ============================================================================

// 构造函数1：从Traits创建
GraphicsWindowQt::GraphicsWindowQt( osg::GraphicsContext::Traits* traits, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
	:   _realized(false)
{

	_widget = NULL;
	_traits = traits;
	init( parent, shareWidget, f );
}

// 构造函数2：从已有GLWidget创建
GraphicsWindowQt::GraphicsWindowQt( GLWidget* widget )
	:   _realized(false)
{
	_widget = widget;
	_traits = _widget ? createTraits( _widget ) : new osg::GraphicsContext::Traits;
	init( NULL, NULL, 0 );
}

// 析构函数
GraphicsWindowQt::~GraphicsWindowQt()
{
	close();

	// 移除GLWidget中的引用
	if ( _widget )
		_widget->_gw = NULL;
}

/**
 * @brief 初始化图形窗口
 * @param parent 父widget
 * @param shareWidget 共享的GL widget
 * @param f 窗口标志
 * @return 是否成功
 */
bool GraphicsWindowQt::init( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
{
	// 从WindowData更新_widget和parent
	WindowData* windowData = _traits.get() ? dynamic_cast<WindowData*>(_traits->inheritedWindowData.get()) : 0;
	if ( !_widget )
		_widget = windowData ? windowData->_widget : NULL;
	if ( !parent )
		parent = windowData ? windowData->_parent : NULL;

	// 如果widget不存在则创建
	_ownsWidget = _widget == NULL;
	if ( !_widget )
	{
		// shareWidget
		if ( !shareWidget ) {
			GraphicsWindowQt* sharedContextQt = dynamic_cast<GraphicsWindowQt*>(_traits->sharedContext.get());
			if ( sharedContextQt )
				shareWidget = sharedContextQt->getGLWidget();
		}

		// WindowFlags
		Qt::WindowFlags flags = f | Qt::Window | Qt::CustomizeWindowHint;
		if ( _traits->windowDecoration )
			flags |= Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint
#if (QT_VERSION_CHECK(4, 5, 0) <= QT_VERSION)
			| Qt::WindowCloseButtonHint
#endif
			;

		// create widget
		_widget = new GLWidget( traits2qglFormat( _traits.get() ), parent, shareWidget, flags );
	}

	// set widget name and position
	// (do not set it when we inherited the widget)
	if ( _ownsWidget )
	{
		_widget->setWindowTitle( _traits->windowName.c_str() );
		_widget->move( _traits->x, _traits->y );
		if ( !_traits->supportsResize ) _widget->setFixedSize( _traits->width, _traits->height );
		else _widget->resize( _traits->width, _traits->height );
	}

	// initialize widget properties
	_widget->setAutoBufferSwap( false );
	_widget->setMouseTracking( true );
	_widget->setFocusPolicy( Qt::WheelFocus );
	_widget->setGraphicsWindow( this );
	useCursor( _traits->useCursor );

	// initialize State
	setState( new osg::State );
	getState()->setGraphicsContext(this);

	// initialize contextID
	if ( _traits.valid() && _traits->sharedContext.valid() )
	{
		getState()->setContextID( _traits->sharedContext->getState()->getContextID() );
		incrementContextIDUsageCount( getState()->getContextID() );
	}
	else
	{
		getState()->setContextID( osg::GraphicsContext::createNewContextID() );
	}

	// 确保事件队列有正确的窗口矩形大小和输入范围
	getEventQueue()->getCurrentEventState()->getGraphicsContext();
	return true;
}

/**
 * @brief 将OSG Traits转换为QGLFormat
 * @param traits OSG图形上下文特性
 * @return QGL格式
 */
QGLFormat GraphicsWindowQt::traits2qglFormat( const osg::GraphicsContext::Traits* traits )
{
	QGLFormat format( QGLFormat::defaultFormat() );

	// 设置缓冲区大小
	format.setAlphaBufferSize( traits->alpha );
	format.setRedBufferSize( traits->red );
	format.setGreenBufferSize( traits->green );
	format.setBlueBufferSize( traits->blue );
	format.setDepthBufferSize( traits->depth );
	format.setStencilBufferSize( traits->stencil );
	format.setSampleBuffers( traits->sampleBuffers );
	format.setSamples( traits->samples );

	// 设置缓冲区启用状态
	format.setAlpha( traits->alpha>0 );
	format.setDepth( traits->depth>0 );
	format.setStencil( traits->stencil>0 );
	format.setDoubleBuffer( traits->doubleBuffer );
	format.setSwapInterval( traits->vsync ? 1 : 0 );
	format.setStereo( traits->quadBufferStereo ? 1 : 0 );

	return format;
}


/**
 * @brief 将QGLFormat转换为OSG Traits
 * @param format QGL格式
 * @param traits OSG图形上下文特性
 */
void GraphicsWindowQt::qglFormat2traits( const QGLFormat& format, osg::GraphicsContext::Traits* traits )
{
	// 读取颜色缓冲区大小
	traits->red = format.redBufferSize();
	traits->green = format.greenBufferSize();
	traits->blue = format.blueBufferSize();
	traits->alpha = format.alpha() ? format.alphaBufferSize() : 0;
	traits->depth = format.depth() ? format.depthBufferSize() : 0;
	traits->stencil = format.stencil() ? format.stencilBufferSize() : 0;

	// 读取多重采样设置
	traits->sampleBuffers = format.sampleBuffers() ? 1 : 0;
	traits->samples = format.samples();

	// 读取显示模式
	traits->quadBufferStereo = format.stereo();
	traits->doubleBuffer = format.doubleBuffer();

	// 读取垂直同步设置
	traits->vsync = format.swapInterval() >= 1;
}

/**
 * @brief 从QGLWidget创建OSG Traits
 * @param widget GL widget
 * @return OSG Traits指针
 */
osg::GraphicsContext::Traits* GraphicsWindowQt::createTraits( const QGLWidget* widget )
{
	osg::GraphicsContext::Traits *traits = new osg::GraphicsContext::Traits;

	// 从widget格式转换
	qglFormat2traits( widget->format(), traits );

	// 从widget几何形状获取窗口信息
	QRect r = widget->geometry();
	traits->x = r.x();
	traits->y = r.y();
	traits->width = r.width();
	traits->height = r.height();

	// 从widget属性获取窗口信息
	traits->windowName = widget->windowTitle().toLocal8Bit().data();
	Qt::WindowFlags f = widget->windowFlags();
	traits->windowDecoration = ( f & Qt::WindowTitleHint ) &&
		( f & Qt::WindowMinMaxButtonsHint ) &&
		( f & Qt::WindowSystemMenuHint );
	QSizePolicy sp = widget->sizePolicy();
	traits->supportsResize = sp.horizontalPolicy() != QSizePolicy::Fixed ||
		sp.verticalPolicy() != QSizePolicy::Fixed;

	return traits;
}



/**
 * @brief 设置窗口矩形位置和大小
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @return 是否成功
 */
bool GraphicsWindowQt::setWindowRectangleImplementation( int x, int y, int width, int height )
{
	if ( _widget == NULL )
		return false;

	_widget->setGeometry( x, y, width, height );
	return true;
}

/**
 * @brief 获取窗口矩形位置和大小
 * @param x X坐标（输出）
 * @param y Y坐标（输出）
 * @param width 宽度（输出）
 * @param height 高度（输出）
 */
void GraphicsWindowQt::getWindowRectangle( int& x, int& y, int& width, int& height )
{
	if ( _widget )
	{
		const QRect& geom = _widget->geometry();
		x = geom.x();
		y = geom.y();
		width = geom.width();
		height = geom.height();
	}
}

/**
 * @brief 设置窗口装饰（标题栏、最小化/最大化按钮等）
 * @param windowDecoration 是否启用装饰
 * @return 是否成功
 */
bool GraphicsWindowQt::setWindowDecorationImplementation( bool windowDecoration )
{
	Qt::WindowFlags flags = Qt::Window|Qt::CustomizeWindowHint;//|Qt::WindowStaysOnTopHint;
	if ( windowDecoration )
		flags |= Qt::WindowTitleHint|Qt::WindowMinMaxButtonsHint|Qt::WindowSystemMenuHint;
	_traits->windowDecoration = windowDecoration;

	if ( _widget )
	{
		_widget->setWindowFlags( flags );

		return true;
	}

	return false;
}

/**
 * @brief 获取窗口装饰状态
 * @return 是否启用装饰
 */
bool GraphicsWindowQt::getWindowDecoration() const
{
	return _traits->windowDecoration;
}

/**
 * @brief 获取焦点
 */
void GraphicsWindowQt::grabFocus()
{
	if ( _widget )
		_widget->setFocus( Qt::ActiveWindowFocusReason );
}

/**
 * @brief 如果鼠标在窗口内则获取焦点
 */
void GraphicsWindowQt::grabFocusIfPointerInWindow()
{
	if ( _widget->underMouse() )
		_widget->setFocus( Qt::ActiveWindowFocusReason );
}

/**
 * @brief 提升窗口到前台
 */
void GraphicsWindowQt::raiseWindow()
{
	if ( _widget )
		_widget->raise();
}

/**
 * @brief 设置窗口名称
 * @param name 窗口名称
 */
void GraphicsWindowQt::setWindowName( const std::string& name )
{
	if ( _widget )
		_widget->setWindowTitle( name.c_str() );
}

/**
 * @brief 获取窗口名称
 * @return 窗口名称
 */
std::string GraphicsWindowQt::getWindowName()
{
	return _widget ? _widget->windowTitle().toStdString() : "";
}

/**
 * @brief 启用或禁用光标
 * @param cursorOn 是否显示光标
 */
void GraphicsWindowQt::useCursor( bool cursorOn )
{
	if ( _widget )
	{
		_traits->useCursor = cursorOn;
		if ( !cursorOn ) _widget->setCursor( Qt::BlankCursor );
		else _widget->setCursor( _currentCursor );
	}
}

/**
 * @brief 设置光标形状
 * @param cursor 光标类型
 */
void GraphicsWindowQt::setCursor( MouseCursor cursor )
{
	if ( cursor==InheritCursor && _widget )
	{
		_widget->unsetCursor();
	}

	switch ( cursor )
	{
	case NoCursor: _currentCursor = Qt::BlankCursor; break;
	case RightArrowCursor: case LeftArrowCursor: _currentCursor = Qt::ArrowCursor; break;
	case InfoCursor: _currentCursor = Qt::SizeAllCursor; break;
	case DestroyCursor: _currentCursor = Qt::ForbiddenCursor; break;
	case HelpCursor: _currentCursor = Qt::WhatsThisCursor; break;
	case CycleCursor: _currentCursor = Qt::ForbiddenCursor; break;
	case SprayCursor: _currentCursor = Qt::SizeAllCursor; break;
	case WaitCursor: _currentCursor = Qt::WaitCursor; break;
	case TextCursor: _currentCursor = Qt::IBeamCursor; break;
	case CrosshairCursor: _currentCursor = Qt::CrossCursor; break;
	case HandCursor: _currentCursor = Qt::OpenHandCursor; break;
	case UpDownCursor: _currentCursor = Qt::SizeVerCursor; break;
	case LeftRightCursor: _currentCursor = Qt::SizeHorCursor; break;
	case TopSideCursor: case BottomSideCursor: _currentCursor = Qt::UpArrowCursor; break;
	case LeftSideCursor: case RightSideCursor: _currentCursor = Qt::SizeHorCursor; break;
	case TopLeftCorner: _currentCursor = Qt::SizeBDiagCursor; break;
	case TopRightCorner: _currentCursor = Qt::SizeFDiagCursor; break;
	case BottomRightCorner: _currentCursor = Qt::SizeBDiagCursor; break;
	case BottomLeftCorner: _currentCursor = Qt::SizeFDiagCursor; break;
	default: break;
	};
	if ( _widget ) _widget->setCursor( _currentCursor );
}


/**
 * @brief 检查窗口是否有效
 * @return 是否有效
 */
bool GraphicsWindowQt::valid() const
{
	return _widget && _widget->isValid();
}

/**
 * @brief 实现窗口Realize操作
 * @return 是否成功
 * 
 * 功能：初始化OpenGL上下文并使其成为当前上下文
 */
bool GraphicsWindowQt::realizeImplementation()
{
	// 保存当前的上下文（只能保存Qt基于的上下文）
	const QGLContext *savedContext = QGLContext::currentContext();

	// 为widget初始化GL上下文
	if ( !valid() )
		_widget->glInit();

	// make current
	_realized = true;
	bool result = makeCurrent();
	_realized = false;

	// fail if we do not have current context
	if ( !result )
	{
		if ( savedContext )
			const_cast< QGLContext* >( savedContext )->makeCurrent();

		OSG_WARN << "Window realize: Can make context current." << std::endl;
		return false;
	}

	_realized = true;

	// make sure the event queue has the correct window rectangle size and input range
	//getEventQueue()->syncWindowRectangleWithGraphicsContext();
	getEventQueue()->getCurrentEventState()->getGraphicsContext();

	// make this window's context not current
	// note: this must be done as we will probably make the context current from another thread
	//       and it is not allowed to have one context current in two threads
	if( !releaseContext() )
		OSG_WARN << "Window realize: Can not release context." << std::endl;

	// restore previous context
	if ( savedContext )
		const_cast< QGLContext* >( savedContext )->makeCurrent();

	return true;
}


/**
 * @brief 检查窗口是否已Realize
 * @return 是否已Realize
 */
bool GraphicsWindowQt::isRealizedImplementation() const
{
	return _realized;
}

/**
 * @brief 关闭窗口实现
 */
void GraphicsWindowQt::closeImplementation()
{
	if ( _widget )
		_widget->close();
	_realized = false;
}

/**
 * @brief 运行图形操作
 * 在图形线程中，这是执行图形操作前最后的机会做有用的事情
 */
void GraphicsWindowQt::runOperations()
{
	// 处理延迟的事件
	if (_widget->getNumDeferredEvents() > 0)
		_widget->processDeferredEvents();

	// 如果当前上下文不是widget的上下文，则设置为当前
	if (QGLContext::currentContext() != _widget->context())
		_widget->makeCurrent();

	GraphicsWindow::runOperations();
}

/**
 * @brief 使上下文成为当前上下文
 * @return 是否成功
 */
bool GraphicsWindowQt::makeCurrentImplementation()
{
	// 处理延迟的事件
	if (_widget->getNumDeferredEvents() > 0)
		_widget->processDeferredEvents();

	_widget->makeCurrent();

	return true;
}

/**
 * @brief 释放当前上下文
 * @return 是否成功
 */
bool GraphicsWindowQt::releaseContextImplementation()
{
	_widget->doneCurrent();
	return true;
}

/**
 * @brief 交换缓冲区实现
 * 
 * FIXME: processDeferredEvents应该真正在GUI（主）线程上下文中执行
 * 但目前找不到可靠的方法。现在只能希望不会有任何*仅GUI线程的操作*
 * 在QGLWidget::event处理器中执行。另一方面，在QGLWidget事件处理器中
 * 调用仅GUI线程操作是Qt的一个bug
 */
void GraphicsWindowQt::swapBuffersImplementation()
{
	if (_widget->getNumDeferredEvents() > 0)
		_widget->processDeferredEvents();

	// 需要在这里调用makeCurrent以恢复我们之前的当前上下文
	// 该上下文可能被processDeferredEvents函数改变
	_widget->makeCurrent();
	_widget->swapBuffers();
}

/**
 * @brief 请求移动鼠标指针
 * @param x X坐标
 * @param y Y坐标
 */
void GraphicsWindowQt::requestWarpPointer( float x, float y )
{
	if ( _widget )
		QCursor::setPos( _widget->mapToGlobal(QPoint((int)x,(int)y)) );
}

// ============================================================================
// QtWindowingSystem类：Qt窗口系统接口（内部实现）
// ============================================================================
// 功能：实现OSG窗口系统接口，用于创建和管理Qt图形上下文
// ============================================================================
class QtWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface
{
public:
	QtWindowingSystem()
	{
		OSG_INFO << "QtWindowingSystemInterface()" << std::endl;
	}

	~QtWindowingSystem()
	{
		if (osg::Referenced::getDeleteHandler())
		{
			osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
			osg::Referenced::getDeleteHandler()->flushAll();
		}
	}

	static QtWindowingSystem* getInterface()
	{
		static QtWindowingSystem* qtInterface = new QtWindowingSystem;
		return qtInterface;
	}

	virtual unsigned int getNumScreens( const osg::GraphicsContext::ScreenIdentifier& /*si*/ )
	{
		OSG_WARN << "osgQt: getNumScreens() not implemented yet." << std::endl;
		return 0;
	}

	virtual void getScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*si*/, osg::GraphicsContext::ScreenSettings & /*resolution*/ )
	{
		OSG_WARN << "osgQt: getScreenSettings() not implemented yet." << std::endl;
	}

	virtual bool setScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*si*/, const osg::GraphicsContext::ScreenSettings & /*resolution*/ )
	{
		OSG_WARN << "osgQt: setScreenSettings() not implemented yet." << std::endl;
		return false;
	}

	virtual void enumerateScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*screenIdentifier*/, osg::GraphicsContext::ScreenSettingsList & /*resolution*/ )
	{
		OSG_WARN << "osgQt: enumerateScreenSettings() not implemented yet." << std::endl;
	}

	virtual osg::GraphicsContext* createGraphicsContext( osg::GraphicsContext::Traits* traits )
	{
		if (traits->pbuffer)
		{
			OSG_WARN << "osgQt: createGraphicsContext - pbuffer not implemented yet." << std::endl;
			return NULL;
		}
		else
		{
			osg::ref_ptr< GraphicsWindowQt > window = new GraphicsWindowQt( traits );
			if (window->valid()) return window.release();
			else return NULL;
		}
	}

private:
	QtWindowingSystem( const QtWindowingSystem& );
	QtWindowingSystem& operator=( const QtWindowingSystem& );
};

// @endcond

#if OSG_VERSION_GREATER_OR_EQUAL(3, 5, 6)
REGISTER_WINDOWINGSYSTEMINTERFACE(Qt, QtWindowingSystem)
#else

// declare C entry point for static compilation.
extern "C" void graphicswindow_Qt(void)
{
	osg::GraphicsContext::setWindowingSystemInterface(QtWindowingSystem::getInterface());
}


void osgQt::initQtWindowingSystem()
{
	graphicswindow_Qt();
}
#endif


/**
 * @brief 设置OSG查看器
 * @param viewer OSG查看器
 */
void osgQt::setViewer( osgViewer::ViewerBase *viewer )
{
	HeartBeat::instance()->init( viewer );
}

/**
 * @brief HeartBeat构造函数
 * 必须在主线程中调用
 */
HeartBeat::HeartBeat() : _timerId( 0 )
{
}

/**
 * @brief HeartBeat析构函数
 * 必须在主线程中调用
 */
HeartBeat::~HeartBeat()
{
	stopTimer();
}

/**
 * @brief 获取单例实例
 * @return HeartBeat实例指针
 */
HeartBeat* HeartBeat::instance()
{
	if (!heartBeat)
	{
		heartBeat = new HeartBeat();
	}
	return heartBeat;
}

/**
 * @brief 停止定时器
 */
void HeartBeat::stopTimer()
{
	if ( _timerId != 0 )
	{
		killTimer( _timerId );
		_timerId = 0;
	}
}

/**
 * @brief 初始化查看器循环
 * @param viewer OSG查看器
 * 
 * 必须在主线程中调用
 */
void HeartBeat::init( osgViewer::ViewerBase *viewer )
{
	if( _viewer == viewer )
		return;

	stopTimer();

	_viewer = viewer;

	if( viewer )
	{
		// 启动定时器，间隔为0表示尽可能快地触发
		_timerId = startTimer( 0 );
		_lastFrameStartTime.setStartTick( 0 );
	}
}


/**
 * @brief 定时器事件处理函数
 * @param event 定时器事件（未使用）
 * 
 * 功能：驱动OSG场景渲染的主循环
 */
void HeartBeat::timerEvent( QTimerEvent * /*event*/ )
{
	osg::ref_ptr< osgViewer::ViewerBase > viewer;
	// 尝试锁定查看器，如果已被删除则停止定时器
	if( !_viewer.lock( viewer ) )
	{
		stopTimer();
		return;
	}

	// 限制帧率
	if( viewer->getRunMaxFrameRate() > 0.0)
	{
		// 如果时间间隔小于最小帧时间，则休眠一段时间
		double dt = _lastFrameStartTime.time_s();
		double minFrameTime = 1.0 / viewer->getRunMaxFrameRate();
		if (dt < minFrameTime)
			OpenThreads::Thread::microSleep(static_cast<unsigned int>(1000000.0*(minFrameTime-dt)));
	}
	else
	{
		// 在按需模式下避免CPU过载
		if( viewer->getRunFrameScheme() == osgViewer::ViewerBase::ON_DEMAND )
		{
			double dt = _lastFrameStartTime.time_s();
			if (dt < 0.01)
				OpenThreads::Thread::microSleep(static_cast<unsigned int>(1000000.0*(0.01-dt)));
		}

		// 记录帧开始时间
		_lastFrameStartTime.setStartTick();

		// 渲染一帧
		if( viewer->getRunFrameScheme() == osgViewer::ViewerBase::ON_DEMAND )
		{
			// 按需模式：只有需要时才渲染
			if( viewer->checkNeedToDoFrame() )
			{
				viewer->frame();
			}
		}
		else
		{
			// 连续模式：持续渲染
			viewer->frame();
		}
	}
}


