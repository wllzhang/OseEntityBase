#include "QWidgetImage.h"

#include <QLayout>

namespace osgQt
{
	/**
	 * @brief QWidgetImage构造函数
	 * @param widget 要渲染的Qt Widget
	 */
	QWidgetImage::QWidgetImage( QWidget* widget )
	{
		// 确保在创建widget之前有有效的QApplication
		getOrCreateQApplication();

		_widget = widget;
		_adapter = new QGraphicsViewAdapter(this, _widget.data());
	}

	QWidgetImage::~QWidgetImage(void)
	{
	}

	/**
	 * @brief 发送焦点提示
	 * @param focus 是否有焦点
	 * @return 是否成功
	 */
	bool QWidgetImage::sendFocusHint(bool focus)
	{
		QFocusEvent event(focus ? QEvent::FocusIn : QEvent::FocusOut, Qt::OtherFocusReason);
		QCoreApplication::sendEvent(_widget, &event);
		return true;
	}

	/**
	 * @brief 清空写缓冲区
	 */
	void QWidgetImage::clearWriteBuffer()
	{
		_adapter->clearWriteBuffer();
	}

	/**
	 * @brief 渲染widget到图像
	 * 
	 * 如果适配器需要渲染，则调用渲染函数
	 */
	void QWidgetImage::render()
	{
		if (_adapter->requiresRendering()) _adapter->render();
	}

	/**
	 * @brief 缩放图像
	 * @param s 宽度
	 * @param t 高度
	 * @param r 深度（未使用）
	 * @param newDataType 数据类型（未使用）
	 */
	void QWidgetImage::scaleImage(int s,int t,int /*r*/, GLenum /*newDataType*/)
	{
		_adapter->resize(s, t);
	}

	/**
	 * @brief 设置最后渲染的帧
	 * @param frameStamp 帧戳
	 */
	void QWidgetImage::setFrameLastRendered(const osg::FrameStamp* frameStamp)
	{
		_adapter->setFrameLastRendered(frameStamp);
	}

	/**
	 * @brief 发送指针事件
	 * @param x X坐标
	 * @param y Y坐标
	 * @param buttonMask 按钮掩码
	 * @return 是否成功
	 */
	bool QWidgetImage::sendPointerEvent(int x, int y, int buttonMask)
	{
		return _adapter->sendPointerEvent(x,y,buttonMask);
	}

	/**
	 * @brief 发送键盘事件
	 * @param key 按键码
	 * @param keyDown 是否按下
	 * @return 是否成功
	 */
	bool QWidgetImage::sendKeyEvent(int key, bool keyDown)
	{
		return _adapter->sendKeyEvent(key, keyDown);
	}



}


