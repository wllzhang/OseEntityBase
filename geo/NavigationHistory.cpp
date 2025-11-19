/**
 * @file NavigationHistory.cpp
 * @brief 导航历史管理类实现文件
 * 
 * 实现NavigationHistory类的所有功能
 */

#include "NavigationHistory.h"
#include <QDebug>
#include <QDateTime>
#include <osgEarth/Units>

// 辅助函数：比较两个视角是否相同
static bool viewpointsEqual(const osgEarth::Viewpoint& vp1, const osgEarth::Viewpoint& vp2)
{
    if (vp1.name() != vp2.name()) {
        return false;
    }
    if (vp1.focalPoint().isSet() != vp2.focalPoint().isSet()) {
        return false;
    }
    if (vp1.focalPoint().isSet() && vp2.focalPoint().isSet()) {
        const auto& fp1 = vp1.focalPoint().value();
        const auto& fp2 = vp2.focalPoint().value();
        // 比较位置是否相同（允许小的误差）
        if (qAbs(fp1.x() - fp2.x()) > 1e-6 || qAbs(fp1.y() - fp2.y()) > 1e-6) {
            return false;
        }
    }
    return true;
}

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
    // 检查是否与上一个视角相似（避免重复记录）
    if (!backStack_.isEmpty()) {
        const auto& lastVp = backStack_.top();
        if (viewpointsEqual(viewpoint, lastVp)) {
            // 与上一个视角相同，不记录
            qDebug() << "视角与上一个相同，跳过记录";
            return;
        }
        
        // 检查视角是否相似（位置变化小于阈值）
        if (viewpoint.focalPoint().isSet() && lastVp.focalPoint().isSet()) {
            const auto& fp1 = viewpoint.focalPoint().value();
            const auto& fp2 = lastVp.focalPoint().value();
            
            // 计算位置差异（经纬度差值，转换为大概的米数）
            double latDiff = qAbs(fp1.y() - fp2.y());
            double lonDiff = qAbs(fp1.x() - fp2.x());
            
            // 大约1度经度 = 111km * cos(纬度)，1度纬度 = 111km
            // 这里使用简化的阈值：如果位置变化小于0.001度（约111米），认为相似
            const double SIMILAR_THRESHOLD = 0.001; // 约111米
            
            // 检查距离和角度是否也相似
            bool rangeSimilar = true;
            if (viewpoint.range().isSet() && lastVp.range().isSet()) {
                double range1 = viewpoint.range().value().as(osgEarth::Units::METERS);
                double range2 = lastVp.range().value().as(osgEarth::Units::METERS);
                // 如果视距变化超过10%，认为不同
                if (qAbs(range1 - range2) / qMax(range1, range2) > 0.1) {
                    rangeSimilar = false;
                }
            }
            
            // 如果位置变化很小且视距相似，认为视角相似，不记录
            if (latDiff < SIMILAR_THRESHOLD && lonDiff < SIMILAR_THRESHOLD && rangeSimilar) {
                qDebug() << "视角与上一个相似（位置变化<111米），跳过记录";
                return;
            }
        }
    }
    
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

QList<NavigationHistory::HistoryItem> NavigationHistory::getAllHistory(const osgEarth::Viewpoint& currentViewpoint) const
{
    QList<HistoryItem> result;
    
    // 将 backStack 转换为列表（最旧的在前面）
    QList<osgEarth::Viewpoint> backList;
    QStack<osgEarth::Viewpoint> backStackCopy = backStack_;
    while (!backStackCopy.isEmpty()) {
        backList.prepend(backStackCopy.pop());
    }
    
    // 添加后退历史记录
    for (int i = 0; i < backList.size(); ++i) {
        HistoryItem item;
        item.viewpoint = backList[i];
        item.index = i;
        item.isCurrent = false;
        
        // 生成显示名称
        QString name = QString::fromStdString(backList[i].name().getOrUse(""));
        if (name.isEmpty()) {
            name = QString("视角 %1").arg(i + 1);
        }
        item.displayName = name;
        result.append(item);
    }
    
    // 添加当前视角（标记为当前）
    HistoryItem currentItem;
    currentItem.viewpoint = currentViewpoint;
    currentItem.index = backList.size();
    currentItem.isCurrent = true;
    QString currentName = QString::fromStdString(currentViewpoint.name().getOrUse(""));
    if (currentName.isEmpty()) {
        currentName = "当前视角";
    }
    currentItem.displayName = currentName;
    result.append(currentItem);
    
    // 添加前进历史记录
    QList<osgEarth::Viewpoint> forwardList;
    QStack<osgEarth::Viewpoint> forwardStackCopy = forwardStack_;
    while (!forwardStackCopy.isEmpty()) {
        forwardList.append(forwardStackCopy.pop());
    }
    
    for (int i = 0; i < forwardList.size(); ++i) {
        HistoryItem item;
        item.viewpoint = forwardList[i];
        item.index = backList.size() + 1 + i;
        item.isCurrent = false;
        
        // 生成显示名称
        QString name = QString::fromStdString(forwardList[i].name().getOrUse(""));
        if (name.isEmpty()) {
            name = QString("视角 %1").arg(backList.size() + 2 + i);
        }
        item.displayName = name;
        result.append(item);
    }
    
    return result;
}

bool NavigationHistory::isViewpointInHistory(const osgEarth::Viewpoint& viewpoint) const
{
    // 检查是否在后退栈中
    QStack<osgEarth::Viewpoint> backStackCopy = backStack_;
    while (!backStackCopy.isEmpty()) {
        const auto& vp = backStackCopy.pop();
        if (viewpointsEqual(vp, viewpoint)) {
            return true;
        }
    }
    
    // 检查是否在前进栈中
    QStack<osgEarth::Viewpoint> forwardStackCopy = forwardStack_;
    while (!forwardStackCopy.isEmpty()) {
        const auto& vp = forwardStackCopy.pop();
        if (viewpointsEqual(vp, viewpoint)) {
            return true;
        }
    }
    
    return false;
}

bool NavigationHistory::jumpToViewpoint(const osgEarth::Viewpoint& currentViewpoint, const osgEarth::Viewpoint& targetViewpoint)
{
    // 如果目标视角在历史记录中，不保存当前视角，直接返回true
    if (isViewpointInHistory(targetViewpoint)) {
        return true;
    }
    
    // 目标视角不在历史记录中，将当前视角保存到后退栈，清除前进栈
    pushViewpoint(currentViewpoint);
    return false;
}

