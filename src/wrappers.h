#ifndef _INCLUDE_WRAPPERS_H_
#define _INCLUDE_WRAPPERS_H_


// ref: https://partner.steamgames.com/downloads/list
// Left 4 Dead - sdk v1.06
// Left 4 Dead 2 - sdk v1.37

class CDetour;

class CFrameSnapshotManager
{
public:
	static int offset_m_PackedEntitiesPool;

	static void* pfn_RecordStringTables;
	static void* pfn_RecordServerClasses;

	static void* pfn_LevelChanged;

	static CDetour* detour_LevelChanged;

	CClassMemoryPoolExt<PackedEntity>& m_PackedEntitiesPool()
	{
		return *reinterpret_cast<CClassMemoryPoolExt<PackedEntity>*>(reinterpret_cast<byte*>(this) + offset_m_PackedEntitiesPool);
	}
};

#endif // _INCLUDE_WRAPPERS_H_
