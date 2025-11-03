/**
 * @file QWidgetImage.h
 * @brief QWidgetImage头文件
 * 
 * 功能：将Qt Widget渲染为OSG Image
 * 用途：在OSG场景中使用Qt GUI元素
 */

#pragma once

#ifndef QWIDGETIMAGE_H
#define QWIDGETIMAGE_H

#include "QGraphicsViewAdapter.h"
#include <osg/Image>

namespace osgQt
{
	/**
	 * @brief QWidgetImage类：将Qt Widget渲染为OSG Image
	 * 
	 * 继承自osg::Image，提供在OSG场景中渲染Qt Widget的能力
	 */
	class QWidgetImage : public osg::Image
	{
	public:
		/**
		 * @brief 构造函数
		 * @param widget 要渲染的Qt Widget
		 */
		QWidgetImage( QWidget* widget=0 );
		~QWidgetImage(void);

		// 获取Qt Widget指针
		QWidget* getQWidget() { return _widget; }
		// 获取GraphicsViewAdapter指针
		QGraphicsViewAdapter* getQGraphicsViewAdapter() { return _adapter; }

		// 需要更新调用
		virtual bool requiresUpdateCall() const { return true; }
		// 更新（渲染）
		virtual void update( osg::NodeVisitor* /*nv*/ ) { render(); }

		void clearWriteBuffer();

		void render();


		/// Overridden scaleImage used to catch cases where the image is
		/// fullscreen and the window is resized.
		virtual void scaleImage(int s,int t,int r, GLenum newDataType);

		virtual bool sendFocusHint(bool focus);

		virtual bool sendPointerEvent(int x, int y, int buttonMask);

		virtual bool sendKeyEvent(int key, bool keyDown);

		virtual void setFrameLastRendered(const osg::FrameStamp* frameStamp);

	protected:

		QPointer<QGraphicsViewAdapter>  _adapter;
		QPointer<QWidget>               _widget;
	};
}








#endif