
#include "content/browser/NavigationController.h"

#include "content/browser/WebPage.h"
#include "content/browser/WebPageImpl.h"

#include "third_party/WebKit/Source/platform/weborigin/ReferrerPolicy.h"
#include "third_party/WebKit/Source/platform/weborigin/Referrer.h"
#include "third_party/WebKit/Source/wtf/text/WTFStringUtil.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/platform/WebTraceLocation.h"
#include "third_party/WebKit/public/web/WebSerializedScriptValue.h"

namespace content {

NavigationController::NavigationController(WebPageImpl* page)
{
    m_currentOffset = -1;
    m_page = page;
}

NavigationController::~NavigationController()
{
}

int NavigationController::historyBackListCount()
{
    return m_currentOffset;
}

int NavigationController::historyForwardListCount()
{
    int result = m_items.size() - m_currentOffset - 1;
    return result;
}

void NavigationController::navigate(int offset)
{
    int pos = m_currentOffset + offset;
    if (pos < 0 || pos > (int)(m_items.size() - 1))
        return;
    blink::WebHistoryItem* item = m_items[pos];
#ifdef DEBUG
    String url = item->urlString();
#endif // DEBUG    
    m_page->loadHistoryItem(WebPage::kMainFrameId, *item, blink::WebHistoryDifferentDocumentLoad, blink::WebURLRequest::UseProtocolCachePolicy);
}

void NavigationController::navigateBackForwardSoon(int offset)
{
    blink::Platform::current()->currentThread()->postTask(FROM_HERE, WTF::bind(&NavigationController::navigate, this, offset));
}

int NavigationController::findEntry(const blink::WebHistoryItem& item) const
{
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->urlString() == item.urlString())
            return i;
    }
    return -1;
}

void NavigationController::insertOrReplaceEntry(const blink::WebHistoryItem& item, blink::WebHistoryCommitType type)
{
    blink::WebHistoryItem* historyItem = new blink::WebHistoryItem();
    historyItem->initialize();
    historyItem->setURLString(item.urlString());
    historyItem->setReferrer(item.referrer(), item.referrerPolicy());
    historyItem->setTarget(item.target());
    historyItem->setStateObject(item.stateObject());
    historyItem->setDocumentState(item.documentState());
    historyItem->setScrollRestorationType(item.scrollRestorationType());
    historyItem->setScrollOffset(item.scrollOffset());
    historyItem->setPageScaleFactor(item.pageScaleFactor());
    historyItem->setItemSequenceNumber(item.itemSequenceNumber());
    historyItem->setDocumentSequenceNumber(item.documentSequenceNumber());
    historyItem->setHTTPContentType(item.httpContentType());
    historyItem->setHTTPBody(item.httpBody());

    //     |  
    switch (type) {
    case blink::WebStandardCommit:
        ++m_currentOffset;
        ASSERT(0 <= m_currentOffset && m_currentOffset <= (int)m_items.size());
        if (m_currentOffset == m_items.size()) {
            m_items.append(historyItem);
        } else {
            m_items[m_currentOffset] = historyItem;
            for (size_t i = m_currentOffset + 1; i < m_items.size(); ++i)
                delete m_items[i];
            m_items.resize(m_currentOffset + 1);
        }
        break;
    case blink::WebBackForwardCommit: {
        int entryIndex = findEntry(item);
        if (-1 == entryIndex) {
            ASSERT(false);
            break;
        }
        m_items[entryIndex] = historyItem;
        m_currentOffset = entryIndex;
        break;
    }
    case blink::WebInitialCommitInChildFrame:
        break;
    case blink::WebHistoryInertCommit:
        break;
    default:
        break;
    }
}

using namespace blink;

DEFINE_TRACE(NavigationController)
{
}

}