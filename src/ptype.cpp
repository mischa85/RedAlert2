/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#define INCLUDE_COM
#include "always.h"

#include "ptype.h"

#include "ccini.h"
#include "findmake.h"
#include "globals.h"
#include "incdec.h"
#include "sun.h"
#include "swizzle.h"
#include "tracker.h"
#include "warhead.h"


/// <summary>
/// Constructs a particle type of the specified name.
/// The new type is added to the particle type heap so that the rules parser and the
/// particle system can find it by name later on.
/// </summary>
/// <param name="ininame">The INI name that identifies this particle type.</param>
ParticleTypeClass::ParticleTypeClass(char const * ininame) :
	BASECLASS(ininame),
	NextParticleOffset(0, 0, 0),
	ColorSpeed(0),
	XVelocity(1),
	YVelocity(1),
	MinZVelocity(0),
	ZVelocityRange(1),
	ColorList(),
	StartColor1(),
	StartColor2(),
	Translucent25State(-1),
	Translucent50State(-1),
	MaxDC(0),
	MaxEC(1),
	Warhead(NULL),
	Damage(0),
	StartFrame(0),
	NumLoopFrames(1),
	Translucency(0),
	WindEffect(0),
	Velocity(0),
	Deacc(0),
	Radius(0),
	DeleteOnStateLimit(false),
	EndStateAI(0),
	StartStateAI(0),
	StateAIAdvance(4),
	FinalDamageState(0),
	IsNormalized(false),
	NextParticle(PARTICLE_NONE),
	BehavesLike(BEHAVIOR_NONE)
{
	ParticleTypes.Add(this);
	AbstractTypePtrTracker.Add(this);
	IsSentient = false;
}


/// <summary>
/// Constructs a particle type object without initializing it.
/// This routine is used by the load process, which is about to overwrite the object
/// with the saved data anyway, so any initialization would only be thrown away.
/// </summary>
ParticleTypeClass::ParticleTypeClass(NoInitClass const & x) :
	BASECLASS(x),
	StartColor1(x),
	StartColor2(x)
{

}


/// <summary>
/// Destroys the particle type object.
/// Every reference to this type is severed and the type is removed from the particle
/// type heap before it goes away.
/// </summary>
ParticleTypeClass::~ParticleTypeClass(void)
{
	Detach_This_From_All(this);
	AbstractTypePtrTracker.Delete(this);
	ParticleTypes.Delete(this);
}


/// <summary>
/// Fetches this particle type's settings from the rules database.
/// This routine reads the appearance, motion, damage and state machine values that
/// decide how particles of this type look and behave once the particle system starts
/// spawning them.
/// </summary>
/// <param name="ini">The INI database to read the particle's section from.</param>
/// <returns>bool; Was the particle type data read?</returns>
bool ParticleTypeClass::Read_INI(CCINIClass const & ini)
{
	if (BASECLASS::Read_INI(ini)) {
		ColorList = ini.Get_RGBClass_List(IniName, "ColorList", TypeList<RGBClass>());

		MaxDC = ini.Get_Int(IniName, "MaxDC", MaxDC);
		MaxEC = ini.Get_Int(IniName, "MaxEC", MaxEC);

		Damage = ini.Get_Int(IniName, "Damage", Damage);
		Warhead = TGet_Class(ini, IniName, "Warhead", Warhead);

		StartFrame = ini.Get_Int(IniName, "StartFrame", StartFrame);
		NumLoopFrames = ini.Get_Int(IniName, "NumLoopFrames", NumLoopFrames);

		Translucency = ini.Get_Int(IniName, "Translucency", Translucency);
		WindEffect = ini.Get_Int(IniName, "WindEffect", WindEffect);

		Velocity = ini.Get_Float(IniName, "Velocity", Velocity);
		Deacc = ini.Get_Float(IniName, "Deacc", Deacc);

		Radius = ini.Get_Int(IniName, "Radius", Radius);
		DeleteOnStateLimit = ini.Get_Bool(IniName, "DeleteOnStateLimit", DeleteOnStateLimit);

		EndStateAI = ini.Get_Int(IniName, "EndStateAI", EndStateAI);
		StartStateAI = ini.Get_Int(IniName, "StartStateAI", StartStateAI);
		StateAIAdvance = ini.Get_Int(IniName, "StateAIAdvance", StateAIAdvance);

		Translucent50State = ini.Get_Int(IniName, "Translucent50State", Translucent50State);
		Translucent25State = ini.Get_Int(IniName, "Translucent25State", Translucent25State);

		IsNormalized = ini.Get_Bool(IniName, "Normalized", IsNormalized);
		ColorSpeed = ini.Get_Float(IniName, "ColorSpeed", ColorSpeed);

		XVelocity = ini.Get_Int(IniName, "XVelocity", XVelocity);
		YVelocity = ini.Get_Int(IniName, "YVelocity", YVelocity);
		MinZVelocity = ini.Get_Int(IniName, "MinZVelocity", MinZVelocity);
		ZVelocityRange = ini.Get_Int(IniName, "ZVelocityRange", ZVelocityRange);

		NextParticleOffset = ini.Get_Offset(IniName, "NextParticleOffset", NextParticleOffset);

		StartColor1 = ini.Get_RGBClass(IniName, "StartColor1", StartColor1);
		StartColor2 = ini.Get_RGBClass(IniName, "StartColor2", StartColor2);

		FinalDamageState = ini.Get_Int(IniName, "FinalDamageState", EndStateAI);

		char buffer[32];
		if (ini.Get_String(IniName, "NextParticle", NULL, buffer, sizeof(buffer))) {
			NextParticle = ParticleTypeClass::From_Name(buffer);
		}

		buffer[0] = '\0';
		ini.Get_String(IniName, "BehavesLike", NULL, buffer, sizeof(buffer));
		BehavesLike = Particle_Behavior_From_Name(buffer);

		return(true);
	}
	return(false);
}


/// <summary>
/// Converts a particle name into a particle type number.
/// This routine is used while the rules are being parsed. A name that has not been
/// declared yet gets a particle type created for it, so that the reference can be
/// satisfied whichever order the declarations happen to appear in.
/// </summary>
/// <returns>Returns with the particle type of that name, or PARTICLE_NONE if no particle
/// was named.</returns>
ParticleType ParticleTypeClass::From_Name(char const * name)
{
	if (stricmp(name, "<none>")) {
		for (int index = PARTICLE_FIRST; index < ParticleTypes.Count(); index++) {
			if (stricmp(name, ParticleTypes[index]->Name()) == 0) {
				return(ParticleType(index));
			}
		}
		ParticleTypeClass *ptr = new ParticleTypeClass(name);
		return(ParticleType(ParticleTypes.ID(ptr)));

	}
	return(PARTICLE_NONE);
}


/// <summary>
/// Fetches the name of the specified particle type.
/// </summary>
/// <returns>Returns with a pointer to the particle's name, or NULL if the particle type is
/// not a legal one.</returns>
const char * ParticleTypeClass::Name_From(ParticleType particle)
{
	if ((unsigned)particle < (unsigned)ParticleTypes.Count()) {
		return(ParticleTypes[particle]->Name());
	}
	return(NULL);
}


/// <summary>
/// Adds this particle type's data to a running checksum.
/// This routine is used by the multiplayer synchronization check so that a rules
/// mismatch between the machines in a game can be spotted.
/// </summary>
/// <param name="crc">The checksum engine to submit the data to.</param>
void ParticleTypeClass::Compute_CRC(CRCEngine & crc) const
{
	BASECLASS::Compute_CRC(crc);

	crc(MaxDC);
	crc(MaxEC);
	crc(Damage);
	crc(StartFrame);
	crc(NumLoopFrames);
	crc(Translucency);
	crc(WindEffect);
	crc(Velocity);
	crc(Deacc);
	crc(Radius);
	crc(DeleteOnStateLimit);
	crc(EndStateAI);
	crc(StartStateAI);
	crc(StateAIAdvance);
	crc(Translucent25State);
	crc(Translucent50State);
	crc(IsNormalized);
	crc(NextParticle);
	crc(BehavesLike);
	crc(ColorSpeed);
}


/// <summary>
/// Fetches the class identifier of this object.
/// The save game system uses this identifier to know which kind of object to build
/// when the stream is read back in.
/// </summary>
/// <param name="retval">Pointer to the location to store the class identifier.</param>
/// <returns>Returns with S_OK, or E_POINTER if no storage location was supplied.</returns>
HRESULT STDMETHODCALLTYPE ParticleTypeClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_ParticleTypeClass;
	return(S_OK);
}


/// <summary>
/// Reads this particle type in from the save game stream.
/// The object is rebuilt in place, its warhead reference is swizzled back into a real
/// pointer, the color list is restored, and the normal image is fetched again.
/// </summary>
/// <returns>Returns with S_OK if the particle type was read, otherwise a failure
/// code.</returns>
HRESULT STDMETHODCALLTYPE ParticleTypeClass::Load(IStream * stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) ParticleTypeClass(NoInitClass());

		Swizzle_Pointer(&Warhead);

		int count = 0;

		HRESULT hr = stream->Read(&count, sizeof(count), NULL);
		if (FAILED(hr)) {
			return(hr);
		}

		for (int index = 0; index < count; index++) {
			RGBClass obj;
			hr = stream->Read(&obj, sizeof(obj), NULL);
			if (FAILED(hr)) {
				return(hr);
			}
			ColorList.Add(obj);
		}

		Fetch_Normal_Image();

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Writes this particle type out to the save game stream.
/// The color list is a dynamic vector, so it cannot travel as part of the raw object
/// image and is written out separately.
/// </summary>
/// <param name="cleardirty">Should the object be marked as clean once written?</param>
/// <returns>Returns with S_OK if the particle type was written, otherwise a failure
/// code.</returns>
HRESULT STDMETHODCALLTYPE ParticleTypeClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {
		int index;
		int count = ColorList.Count();

		HRESULT hr = stream->Write(&count, sizeof(count), NULL);
		if (FAILED(hr)) {
			return(hr);
		}

		for (index = 0; index < count; index++) {
			hr = stream->Write(&ColorList[index], sizeof(ColorList[index]), NULL);
			if (FAILED(hr)) {
				return(S_OK);
			}
		}

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Fetches the particle type of the specified name, creating it if necessary.
/// This routine is used by the rules parser so that a particle type can be referred to
/// before its own declaration has been read.
/// </summary>
/// <returns>Returns with a pointer to the particle type that goes by that name.</returns>
ParticleTypeClass * ParticleTypeClass::Find_Or_Make(char const * name)
{
	return(TFind_Or_Make<ParticleTypeClass>(name, ParticleTypes));
}


/// <summary>
/// Removes any reference this particle type has to the specified object.
/// This routine is called when an object is about to disappear so that nothing is left
/// pointing at it. For a particle type, the only such reference is the warhead it
/// inflicts its damage with.
/// </summary>
/// <param name="target">Pointer to the object that is about to be destroyed.</param>
/// <param name="all">Should the detachment be forced?</param>
void ParticleTypeClass::Detach(AbstractClass const * target, bool all)
{
	if (Warhead == target) {
		Warhead = NULL;
	}
}


/// <summary>
/// Converts a particle behavior name into a behavior type.
/// This routine is used when reading the "BehavesLike" entry of a particle declaration.
/// The behavior chosen decides which of the specialized particle logic routines will
/// drive particles of that type.
/// </summary>
/// <returns>Returns with the behavior that matches the name, or BEHAVIOR_NONE if the name
/// is not recognized.</returns>
ParticleBehaviorType Particle_Behavior_From_Name(char const * name)
{
	static char const * _BehaviorNames[BEHAVIOR_COUNT] = {
		"Gas",
		"Smoke",
		"Fire",
		"Spark",
		"Railgun",
		"Web",
		"WeakGas"
	};

	for (ParticleBehaviorType behavior = BEHAVIOR_FIRST; behavior < BEHAVIOR_COUNT; behavior++) {
		if (!strcmpi(name, _BehaviorNames[behavior])) {
			return(behavior);
		}
	}
	return(BEHAVIOR_NONE);
}


/// <summary>
/// Fetches the memory size of this particle type object.
/// This routine is used by the save and load system when it needs to know how much of
/// the raw object image to transfer.
/// </summary>
/// <param name="oldsave">Is the size wanted for an older save game format?</param>
/// <returns>Returns with the size of this object in bytes.</returns>
int ParticleTypeClass::Fetch_Object_Size(void) const // RA2: no argument
{
	return(sizeof(*this));
}


/// <summary>
/// Fetches the run time type identifier of this object.
/// </summary>
/// <returns>Returns with RTTI_PARTICLETYPE.</returns>
RTTIType ParticleTypeClass::Fetch_RTTI(void) const
{
	return(RTTI_PARTICLETYPE);
}


/// <summary>
/// Places an object of this particle type onto the map.
/// This routine is the scenario placement hook. A particle is not a map object, so it
/// cannot be placed this way and the request is always refused.
/// </summary>
/// <returns>bool; Was the particle placed?</returns>
bool ParticleTypeClass::Create_And_Place(Cell const & , HouseClass * house) const
{
	return(false);
}


/// <summary>
/// Creates an object of this particle type.
/// This routine is the generic object type creation hook. Particles are never
/// manufactured through it -- the particle system spawns them directly -- so the
/// request is always refused.
/// </summary>
/// <returns>Returns with a pointer to the object created. This is always NULL.</returns>
ObjectClass * ParticleTypeClass::Create_One_Of(HouseClass *) const
{
	return(NULL);
}
