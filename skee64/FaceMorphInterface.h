#pragma once

#include "skse64/GameTypes.h"
#include "StringTable.h"

#ifdef _DEBUG
//#define _DEBUG_HOOK
//#define _DEBUG_MORPHAPPLICATOR
//#define _DEBUG_DATADUMP
//#define _DEBUG_DATAREADER
//#define _DEBUG_MORPH
#endif

class BSFaceGenNiNode;
class TESNPC;
class SliderArray;
class RaceMenuSlider;
struct SKSESerializationInterface;
class TESRace;
class BGSHeadPart;
class TESForm;
class TESModelTri;

#define SLIDER_OFFSET 200
#define SLIDER_CATEGORY_EXTRA 512
#define SLIDER_CATEGORY_EXPRESSIONS 1024

#define SLIDER_MOD_DIRECTORY "actors\\character\\FaceGenMorphs\\"
#define SLIDER_DIRECTORY "actors\\character\\FaceGenMorphs\\morphs\\"

#define MORPH_CACHE_TEMPLATE "%08X.tri"
#define MORPH_CACHE_DIR "cache\\"
#define MORPH_CACHE_PATH "actors\\character\\FaceGenMorphs\\morphs\\cache\\"

#include <set>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

#include "IHashType.h"
#include "OverrideVariant.h"
#include "IPluginInterface.h"

#include "skse64/GameResources.h"
#include "skse64/GameThreads.h"

class SliderInternal
{
public:
	SliderInternal()
	{
		category = -1;
		name = "";
		lowerBound = "";
		upperBound = "";
		type = 0;
		presetCount = 0;
	}

	void copy(SliderInternal * slider)
	{
		category = slider->category;
		name = slider->name;
		lowerBound = slider->lowerBound;
		upperBound = slider->upperBound;
		type = slider->type;
		presetCount = slider->presetCount;
	}

	enum
	{
		kCategoryExpressions = SLIDER_CATEGORY_EXPRESSIONS,
		kCategoryExtra = SLIDER_CATEGORY_EXTRA,
		kCategoryBody = 4,
		kCategoryHead = 8,
		kCategoryFace = 16,
		kCategoryEyes = 32,
		kCategoryBrow = 64,
		kCategoryMouth = 128,
		kCategoryHair = 256
	};

	enum 
	{
		kTypeSlider = 0,
		kTypePreset = 1,
		kTypeHeadPart = 2
	};

	SInt32			category;
	SKEEFixedString	name;
	SKEEFixedString	lowerBound;
	SKEEFixedString	upperBound;
	UInt8			type;
	UInt8			presetCount;
};

typedef std::shared_ptr<SliderInternal> SliderInternalPtr;

class SliderGender
{
public:
	SliderGender()
	{
		slider[0] = NULL;
		slider[1] = NULL;
	}

	SliderInternalPtr slider[2];
};
typedef std::shared_ptr<SliderGender> SliderGenderPtr;
typedef std::vector<SliderInternalPtr> SliderList;
typedef std::unordered_map<TESRace*, SliderList> RaceSliders;
typedef std::vector<SKEEFixedString> MorphSet;

class MorphMap : public std::map<SKEEFixedString, MorphSet>
{
public:
	class Visitor
	{
	public:
		virtual bool Accept(const SKEEFixedString & morphName) { return false; };
	};
	
	void AddMorph(const SKEEFixedString & key, const SKEEFixedString & value);
	bool Visit(const SKEEFixedString & key, Visitor & visitor);
	void Revert();

#ifdef _DEBUG_DATADUMP
	void DumpMap();
	class DumpVisitor : public Visitor
	{
	public:
		virtual bool Accept(BSFixedString morphName)
		{
			_MESSAGE("Extra Morph: %s", morphName.data);
			return false;
		};
	};
#endif
};

class SliderMap : public std::unordered_map<SKEEFixedString, SliderGenderPtr>
{
public:
	SliderMap() : std::unordered_map<SKEEFixedString, SliderGenderPtr>(){ }

	void AddSlider(const SKEEFixedString & key, UInt8 gender, SliderInternal & slider);

#ifdef _DEBUG_DATADUMP
	void DumpMap();
#endif
};

typedef std::shared_ptr<SliderMap>	SliderMapPtr;


class SliderSet : public std::set<SliderMapPtr>
{
public:
	bool for_each_slider(std::function<bool(SliderGenderPtr)> func);
};

typedef std::shared_ptr<SliderSet> SliderSetPtr;

class RaceMap : public std::unordered_map<TESRace*, SliderSetPtr>
{
public:
	SliderSetPtr GetSliderSet(TESRace * race);
	bool AddSliderMap(TESRace * race, SliderMapPtr sliderMap);
	bool CreateDefaultMap(TESRace * race);

	void Revert();

#ifdef _DEBUG_DATADUMP
	void DumpMap();
#endif
};

class ValueSet : public std::unordered_map<StringTableItem, float>
{
public:
	void SetValue(const SKEEFixedString & name, float value);
	void ClearValue(const SKEEFixedString & name);
	float GetValue(const SKEEFixedString & name);
};

class ValueMap : public std::unordered_map<TESNPC*, ValueSet>
{
public:
	ValueSet * GetValueSet(TESNPC* npc);
	void EraseNPC(TESNPC * npc);

	float GetMorphValueByName(TESNPC* npc, const SKEEFixedString & name);
	void SetMorphValue(TESNPC* npc, const SKEEFixedString & name, float value);
};

#define VERTEX_THRESHOLD 0.00001
#define VERTEX_MULTIPLIER 10000

class MappedSculptData : public std::unordered_map<UInt16, NiPoint3>
{
public:
	void force_insert(value_type const & v)
	{
		if (abs(v.second.x) < VERTEX_THRESHOLD && abs(v.second.y) < VERTEX_THRESHOLD && abs(v.second.z) < VERTEX_THRESHOLD)
			return;

		auto res = insert(v);
		if (!res.second)
			(*res.first).second = v.second;
	}

	void add(value_type const & v)
	{
		auto res = insert(v);
		if (!res.second)
			(*res.first).second += v.second;

		if (abs((*res.first).second.x) < VERTEX_THRESHOLD && abs((*res.first).second.y) < VERTEX_THRESHOLD && abs((*res.first).second.z) < VERTEX_THRESHOLD)
			erase(res.first);
	}
};
typedef std::shared_ptr<MappedSculptData> MappedSculptDataPtr;

class SculptData : public std::unordered_map<StringTableItem, MappedSculptDataPtr>
{
public:
	MappedSculptDataPtr GetSculptHost(SKEEFixedString, bool create = true);

	static SKEEFixedString GetHostByPart(BGSHeadPart * headPart);
};
using SculptDataPtr = std::shared_ptr<SculptData>;

class SculptStorage : public std::unordered_map < TESNPC*, SculptDataPtr >
{
public:
	void SetSculptTarget(TESNPC * npc, SculptDataPtr sculptData);
	SculptDataPtr GetSculptTarget(TESNPC* npc, bool create = true);
	void EraseNPC(TESNPC * npc);
};

class TRIFile
{
public:
	TRIFile()
	{
		vertexCount = -1;
	}

	bool Load(const char * triPath);
	bool Apply(BSGeometry * geometry, SKEEFixedString morph, float relative);

	struct Morph
	{
		SKEEFixedString name;
		float multiplier;
		
		struct Vertex
		{
			SInt16 x, y, z;
		};

		std::vector<Vertex> vertices;
	};

	SInt32 vertexCount;
	std::unordered_map<SKEEFixedString, Morph> morphs;
};

class TRIModelData
{
public:
	TRIModelData()
	{
		vertexCount = -1;
		morphModel = NULL;
	}
	SInt32 vertexCount;
	std::shared_ptr<TRIFile> triFile;
	TESModelTri * morphModel;
};

typedef std::unordered_map<SKEEFixedString, TRIModelData> ModelMap;

class FaceMorphInterface : public IPluginInterface
{
public:
	enum
	{
		kCurrentPluginVersion = 1,
		kSerializationVersion1 = 1,
		kSerializationVersion2 = 2,
		kSerializationVersion = kSerializationVersion2
	};
	virtual skee_u32 GetVersion();

	bool Load(SKSESerializationInterface * intfc, UInt32 version) { return false; } // Unused due to separate dblock name for morph and sculpt
	void Save(SKSESerializationInterface * intfc, UInt32 kVersion);
	virtual void Revert();
	void RevertInternals();

	bool LoadMorphData(SKSESerializationInterface * intfc, UInt32 version, const StringIdMap & stringTable);
	bool LoadSculptData(SKSESerializationInterface * intfc, UInt32 version, const StringIdMap & stringTable);

	void LoadMods();

	virtual float GetMorphValueByName(TESNPC* npc, const SKEEFixedString & name);
	virtual void SetMorphValue(TESNPC* npc, const SKEEFixedString & name, float value);

	void SetMorph(TESNPC * npc, BSFaceGenNiNode * faceNode, const SKEEFixedString & name, float relative);

	void ApplyMorph(TESNPC * npc, BGSHeadPart * headPart, BSFaceGenNiNode * faceNode);
	void ApplyMorphs(TESNPC * npc, BSFaceGenNiNode * faceNode);

	SInt32 LoadSliders(tArray<RaceMenuSlider> * sliderArray, RaceMenuSlider * slider);

	void ReadMorphs(std::string fixedPath, std::string modName, std::string fileName);
	void ReadRaces(std::string fixedPath, std::string modName, std::string fileName);
	SliderMapPtr ReadSliders(std::string fixedPath, std::string modName, std::string fileName);

	SliderInternalPtr GetSlider(TESRace * race, UInt8 gender, SKEEFixedString name);
	SliderInternalPtr GetSliderByIndex(TESRace * race, UInt32 index);

	SliderList * CreateSliderList(TESRace * race, UInt8 gender);
	void AddSlider(TESRace * race, SliderInternalPtr & slider);

	bool VisitMorphMap(const SKEEFixedString & key, MorphMap::Visitor & visitor);

	bool CacheHeadPartModel(BGSHeadPart * headPart, bool cacheTRI = false);
	bool GetModelTri(const SKEEFixedString & filePath, TRIModelData & modelData);
	TRIModelData & GetExtendedModelTri(const SKEEFixedString & morphName, bool cacheTRI = false);

	inline SculptDataPtr GetSculptTarget(TESNPC * npc, bool create = true)
	{
		return m_sculptStorage.GetSculptTarget(npc, create);
	}
	inline void SetSculptTarget(TESNPC * npc, const SculptDataPtr & data)
	{
		return m_sculptStorage.SetSculptTarget(npc, data);
	}
	inline void EraseSculptData(TESNPC * npc)
	{
		m_sculptStorage.EraseNPC(npc);
	}
	inline void EraseMorphData(TESNPC * npc)
	{
		m_valueMap.EraseNPC(npc);
	}

	inline ValueMap& GetValueMap() { return m_valueMap; }

protected:
	SliderList * currentList;
	RaceSliders m_internalMap;
	RaceMap m_raceMap;
	MorphMap m_morphMap;
	ValueMap m_valueMap;
	ModelMap m_modelMap;

	SculptStorage m_sculptStorage;

	friend class RacePartDefaultGen;

#ifdef _DEBUG_DATADUMP
	void DumpAll();
#endif
};

class SKSETaskApplyMorphs : public TaskDelegate
{
public:
	virtual void Run();
	virtual void Dispose() { delete this; }

	SKSETaskApplyMorphs(Actor * actor);

private:
	UInt32			m_formId;
};

class SKSETaskApplyMorphNormals : public TaskDelegate
{
public:
	virtual void Run();
	virtual void Dispose() { delete this; }

	SKSETaskApplyMorphNormals(NiPointer<NiAVObject> faceNode, bool updateModel = true);

protected:
	NiPointer<NiAVObject> m_faceNode;
	bool m_updateModel;
};