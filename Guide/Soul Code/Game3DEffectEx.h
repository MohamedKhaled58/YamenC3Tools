// Game3DEffectEx.h: interface for the CGame3DEffectEx class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GAME3DEFFECTEX_H__2DCBEF81_684D_4AF9_AB9B_029B8E3C6E21__INCLUDED_)
#define AFX_GAME3DEFFECTEX_H__2DCBEF81_684D_4AF9_AB9B_029B8E3C6E21__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "BaseFunc.h"
#include "d3dx8math.h"
class C3DRolePart;
class C3DRole;
struct ShapeBackupInfo;

typedef struct
{
	OBJID idEffect;
	OBJID idTexture;
	ShapeBackupInfo* pShapeInfoBuf;
	int nShapes;
	int	nASB;
	int	nADB;
}Game3DEffectIdInfo;

typedef struct 
{
	Game3DEffectIdInfo* pIDInfo;
	int		nInfoAmount;
	char	szIndex[64];	
	DWORD	dwTimeDelay;
	DWORD	dwCurrentFrameIndex;
	DWORD	dwTimeBegin;
	DWORD	dwFrameInterval;
	DWORD	dwLoopInterval;
	DWORD	dwLoopTime;
	int		nOffsetX;
	int		nOffsetY;
	int		nOffsetZ;
	BOOL	bShow;
	BOOL	bSelfRotate;
	float	fDir;
	int     nShapeAir;
	
}Game3DEffectExInfo;

#include <deque>
using namespace std;
typedef deque<Game3DEffectExInfo*>DEQUE_3DEFFECTEXINFO;

class CGame3DEffectEx  
{
public:
	CGame3DEffectEx();
	virtual ~CGame3DEffectEx();
	
public:
	BOOL Process();
	void Clear();
	void Show(C3DRole* p3DRole, CMyPos posWorld);
	void Show(CMyPos posWorld);
	void Show(C3DRolePart* pPart);
	void Delete(char* pszIndex);
	void DeleteByVMesh(char* pszVMesh);
	void Add(Game3DEffectExInfo* pInfo);
	void Add(char* pszIndex, BOOL bSelfRotate = false, int nDir = 0 ,DWORD dwLook=0);
	void SetPos ( int nX, int nY, int nRotate, float fScale );
	void Rotate(float x, float y, float z);
	void Scale(float x, float y, float z);
	void SetOffset(int nOffset){m_nOffsetY = nOffset;}
	void SetHeight ( float fHeight );

	void SetBlend(int nIndex, int nPartIndex, int nASB, int nADB);
	void GetBlend(int nIndex, int nPartIndex, int &nASB, int &nADB);
	DWORD GetPartAmount(int nIndex);

	void EverPlay();
	void SetOblique(BOOL bOblique){m_bOblique = bOblique;}
	BOOL TestEffect(char* pszIndex);
private:
	int		m_nOffsetY;
	float	m_Height;
	BOOL	m_bOblique;
	float	m_fX, m_fY, m_fZ;
private:
	void Show(Game3DEffectExInfo* pInfo, CMyPos posWorld);
	void Show(Game3DEffectExInfo* pInfo, C3DRolePart* pPart);
	void Show(Game3DEffectExInfo* pInfo,  C3DRolePart* pPart, const char* pszVMesh);
	BOOL Process(Game3DEffectExInfo* pInfo);
private:
	DEQUE_3DEFFECTEXINFO m_setInfo;

};

#endif // !defined(AFX_GAME3DEFFECTEX_H__2DCBEF81_684D_4AF9_AB9B_029B8E3C6E21__INCLUDED_)
