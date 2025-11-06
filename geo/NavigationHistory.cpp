/**
 * @file NavigationHistory.cpp
 * @brief 导航历史管理类实现文件
 * 
 * 实现NavigationHistory类的所有功能
 */

#include "NavigationHistory.h"
#include <QDebug>

NavigationHistory::NavigationHistory(QObject* parent)
    : QObject(parent)
    , maxHistorySize_(50)  // 最多保存50个历史记录
{
}

NavigationHistory::~NavigationHistory()
{
}

void NavigationHistory::pushViewpoint(const osgEarth::Viewpoint& viewpoint)
{
    // 添加到后退栈
    backStack_.push(viewpoint);
    
    // 限制历史记录数量
    while (backStack_.size() > maxHistorySize_) {
        backStack_.remove(0);  // 移除最旧的记录
    }
    
    // 添加新记录时，清除前进栈（因为产生了新的分支）
    forwardStack_.clear();
    
    emitStateChanged();
}

bool NavigationHistory::goBack(const osgEarth::Viewpoint& currentViewpoint, osgEarth::Viewpoint& backViewpoint)
{
    if (backStack_.isEmpty()) {
        return false;
    }
    
    // 将当前视角保存到前进栈
    forwardStack_.push(currentViewpoint);
    
    // 限制前进栈大小
    while (forwardStack_.size() > maxHistorySize_) {
        forwardStack_.remove(0);
    }
    
    // 获取后退的视角
    backViewpoint = backStack_.pop();
    
    emitStateChanged();
    return true;
}

bool NavigationHistory::goForward(const osgEarth::Viewpoint& currentViewpoint, osgEarth::Viewpoint& forwardViewpoint)
{
    if (forwardStack_.isEmpty()) {
        return false;
    }
    
    // 将当前视角保存到后退栈
    backStack_.push(currentViewpoint);
    
    // 限制后退栈大小
    while (backStack_.size() > maxHistorySize_) {
        backStack_.remove(0);
    }
    
    // 获取前进的视角
    forwardViewpoint = forwardStack_.pop();
    
    emitStateChanged();
    return true;
}

void NavigationHistory::clear()
{
    backStack_.clear();
    forwardStack_.clear();
    emitStateChanged();
}

bool NavigationHistory::canGoBack() const
{
    return !backStack_.isEmpty();
}

bool NavigationHistory::canGoForward() const
{
    return !forwardStack_.isEmpty();
}

int NavigationHistory::getHistoryCount() const
{
    return backStack_.size() + forwardStack_.size();
}

void NavigationHistory::emitStateChanged()
{
    emit historyStateChanged(canGoBack(), canGoForward());
}

