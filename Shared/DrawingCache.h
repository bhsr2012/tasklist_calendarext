// DrawingCache.h: 进程级共享绘制缓存
//
// 目的：任务树、任务列表、日历、甘特图这些视图各自 new 一份字体、画刷，并对同一个
// 字符串反复调用 GetTextExtent。把这三类对象和一个文本度量表做成全进程单例后：
//   1. 同规格字体/同色画刷只建一个 GDI 对象（句柄数下降）
//   2. 同一个字符串在同一字体下的宽度只量一次（滚动/重绘时省掉大量 GDI 调用）
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DRAWINGCACHE_H__1F0A3E24_9C77_4E6B_9E33_5B7A2C41D0E8__INCLUDED_)
#define AFX_DRAWINGCACHE_H__1F0A3E24_9C77_4E6B_9E33_5B7A2C41D0E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <map>
#include <string>

//////////////////////////////////////////////////////////////////////

class CDrawingCache
{
public:
	static CDrawingCache& Instance()
	{
		static CDrawingCache s_cache;
		return s_cache;
	}

	// 字体：同一 LOGFONT 全进程共用一个 HFONT
	HFONT GetFont(const LOGFONT& lf)
	{
		std::wstring sKey(GetFontKey(lf));
		std::map<std::wstring, HFONT>::iterator it = m_mapFonts.find(sKey);

		if (it != m_mapFonts.end())
			return it->second;

		HFONT hFont = ::CreateFontIndirect(&lf);

		if (hFont)
			m_mapFonts[sKey] = hFont;

		return hFont;
	}

	HBRUSH GetBrush(COLORREF cr)
	{
		std::map<COLORREF, HBRUSH>::iterator it = m_mapBrushes.find(cr);

		if (it != m_mapBrushes.end())
			return it->second;

		HBRUSH hBrush = ::CreateSolidBrush(cr);

		if (hBrush)
			m_mapBrushes[cr] = hBrush;

		return hBrush;
	}

	// nStyle 为 PS_* 取值
	HPEN GetPen(COLORREF cr, int nWidth = 1, int nStyle = PS_SOLID)
	{
		DWORD dwKey = (DWORD)(cr & 0x00FFFFFF);
		dwKey |= ((DWORD)(nWidth & 0xFF) << 24);
		dwKey |= ((DWORD)(nStyle & 0x0F) << 8);

		std::map<DWORD, HPEN>::iterator it = m_mapPens.find(dwKey);

		if (it != m_mapPens.end())
			return it->second;

		HPEN hPen = ::CreatePen(nStyle, nWidth, cr);

		if (hPen)
			m_mapPens[dwKey] = hPen;

		return hPen;
	}

	// 文本度量：键为 (当前字体, 字符串)
	SIZE GetTextExtent(CDC* pDC, const CString& sText)
	{
		if (pDC == NULL || pDC->GetSafeHdc() == NULL)
		{
			SIZE szZero = { 0, 0 };
			return szZero;
		}

		HFONT hFont = (HFONT)::GetCurrentObject(pDC->GetSafeHdc(), OBJ_FONT);
		std::map<std::wstring, SIZE>& mapFont = m_mapExtents[hFont];
		std::wstring sKey((LPCTSTR)sText);

		std::map<std::wstring, SIZE>::iterator it = mapFont.find(sKey);

		if (it != mapFont.end())
			return it->second;

		// 超过上限整体清空，避免长期运行无限增长
		if (m_nExtentEntries >= EXTENT_CACHE_LIMIT)
		{
			m_mapExtents.clear();
			mapFont = m_mapExtents[hFont];
			m_nExtentEntries = 0;
		}

		SIZE szText = pDC->GetTextExtent(sText);
		mapFont[sKey] = szText;
		m_nExtentEntries++;

		return szText;
	}

	int GetFontCount() const { return (int)m_mapFonts.size(); }
	int GetBrushCount() const { return (int)m_mapBrushes.size(); }
	int GetPenCount() const { return (int)m_mapPens.size(); }
	int GetExtentCount() const { return m_nExtentEntries; }

	void Clear()
	{
		std::map<std::wstring, HFONT>::iterator itFont = m_mapFonts.begin();

		for (; itFont != m_mapFonts.end(); ++itFont)
			::DeleteObject(itFont->second);

		std::map<COLORREF, HBRUSH>::iterator itBrush = m_mapBrushes.begin();

		for (; itBrush != m_mapBrushes.end(); ++itBrush)
			::DeleteObject(itBrush->second);

		std::map<DWORD, HPEN>::iterator itPen = m_mapPens.begin();

		for (; itPen != m_mapPens.end(); ++itPen)
			::DeleteObject(itPen->second);

		m_mapFonts.clear();
		m_mapBrushes.clear();
		m_mapPens.clear();
		m_mapExtents.clear();
		m_nExtentEntries = 0;
	}

protected:
	CDrawingCache() : m_nExtentEntries(0) {}
	~CDrawingCache() { Clear(); }

	CDrawingCache(const CDrawingCache&);
	CDrawingCache& operator=(const CDrawingCache&);

	static const int EXTENT_CACHE_LIMIT = 4000;

	static std::wstring GetFontKey(const LOGFONT& lf)
	{
		wchar_t szKey[LF_FACESIZE + 64];

		_snwprintf_s(szKey, _countof(szKey), _TRUNCATE,
					 L"%s|%d|%d|%d|%d|%d|%d|%d",
					 lf.lfFaceName, lf.lfHeight, lf.lfWeight,
					 lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut,
					 lf.lfCharSet, lf.lfQuality);

		return std::wstring(szKey);
	}

	std::map<std::wstring, HFONT>			m_mapFonts;
	std::map<COLORREF, HBRUSH>				m_mapBrushes;
	std::map<DWORD, HPEN>					m_mapPens;
	std::map<HFONT, std::map<std::wstring, SIZE> >	m_mapExtents;
	int										m_nExtentEntries;
};

#endif // !defined(AFX_DRAWINGCACHE_H__...)
