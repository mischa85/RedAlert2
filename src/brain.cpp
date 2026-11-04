/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#define INCLUDE_COM
#include "always.h"

#include "brain.h"

#include "crc.h"
#include "ftimer.h"
#include "globals.h"
#include "noinit.h"
#include "sun.h"
#include "swizzle.h"
#include "tracker.h"
#include "vector.h"


/// <summary>
/// Creates a neuron.
/// The neuron notes the frame it came into being on and starts out unattached -- a brain
/// adopts it later. It registers itself with the object trackers so that it can be
/// detached cleanly when the things it refers to go away.
/// </summary>
NeuronClass::NeuronClass(void):
	Pointer1(NULL),
	Pointer2(NULL),
	MyBrain(NULL),
	CreationFrame(Frame)
{
	TeamPtrTracker.Add(this);
	AbstractTypePtrTracker.Add(this);
}


/// <summary>
/// Removes this neuron from the object trackers.
/// </summary>
NeuronClass::~NeuronClass(void)
{
	AbstractTypePtrTracker.Delete(this);
	TeamPtrTracker.Delete(this);
}


/// <summary>
/// Fetches the class identifier of this object.
/// This routine is used by the save game system so that it knows what kind of object to
/// construct when the stream is read back in.
/// </summary>
/// <param name="retval">Pointer to the place to store the class identifier.</param>
/// <returns>Returns with S_OK, or E_POINTER if no destination was supplied.</returns>
HRESULT STDMETHODCALLTYPE NeuronClass::GetClassID(CLSID * retval)
{
	if (retval == NULL) return(E_POINTER);
	*retval = CLSID_NeuronClass;
	return(S_OK);
}


/// <summary>
/// Loads this neuron from the save game stream.
/// The object is rebuilt in place once the base class has read the raw image back over
/// it, and the pointers it carries are handed to the swizzler so that they can be
/// remapped onto wherever those objects now live.
/// </summary>
/// <returns>Returns with S_OK if the neuron was read successfully.</returns>
HRESULT STDMETHODCALLTYPE NeuronClass::Load(IStream * stream)
{
	HRESULT result = BASECLASS::Load(stream);
	if (SUCCEEDED(result)) {
		new (this) NeuronClass(NoInitClass());

		Swizzle_Pointer(&Pointer2);
		Swizzle_Pointer(&Pointer1);
		Swizzle_Pointer(&MyBrain);

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Saves this neuron to the save game stream.
/// </summary>
/// <param name="cleardirty">Should the dirty flag be cleared once the object is written?</param>
/// <returns>Returns with S_OK if the neuron was written successfully.</returns>
HRESULT STDMETHODCALLTYPE NeuronClass::Save(IStream * stream, BOOL cleardirty)
{
	HRESULT result = BASECLASS::Save(stream, cleardirty);
	if (SUCCEEDED(result)) {

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Folds this neuron's state into a running CRC.
/// The multiplayer sync check accumulates every object's state each frame; a CRC that
/// differs between machines means the simulations have drifted apart. A neuron
/// contributes its age, since its pointers are meaningless across machines.
/// </summary>
/// <param name="crc">The running CRC to fold this neuron's state into.</param>
void NeuronClass::Compute_CRC(CRCEngine &crc) const
{
	BASECLASS::Compute_CRC(crc);
	crc((int)(Frame - CreationFrame));
}


/// <summary>
/// Destroys the brain along with every neuron it owns.
/// </summary>
BrainClass::~BrainClass(void)
{
	Deinit();
}


/// <summary>
/// Destroys every neuron this brain owns.
/// The brain itself survives and can be filled again afterwards.
/// </summary>
void BrainClass::Deinit(void)
{
	int count = Neurons.Count();
	for (int i = 0; i < count; i++) {
		delete Neurons[i];
		Neurons[i] = NULL;
	}

	Neurons.Clear();
}


/// <summary>
/// Prepares the brain to hold neurons.
/// Any neurons the brain was already holding are thrown away before the new limits take
/// effect, so this routine can be used to start a brain over from scratch.
/// </summary>
/// <param name="min">The fewest neurons this brain is meant to hold.</param>
/// <param name="max">The most neurons this brain will accept.</param>
void BrainClass::Init(int min, int max)
{
	Deinit();
	MinCount = min;
	MaxCount = max;
}


/// <summary>
/// Adds a neuron to this brain.
/// The neuron is taken on only if the brain still has room for it, and it is told which
/// brain now owns it. A neuron that is turned away is left in the caller's hands.
/// </summary>
/// <returns>bool; Was the neuron taken on?</returns>
bool BrainClass::Add_Neuron(NeuronClass *neuron)
{
	if (Neurons.Count() < MaxCount) {
		Neurons.Add(neuron);
		neuron->MyBrain = this;
		return(true);
	}
	return(false);
}


/// <summary>
/// Saves this brain to the save game stream.
/// The number of neurons goes out first so that the loader knows how many to expect,
/// followed by each neuron in turn.
/// </summary>
/// <param name="cleardirty">Should the neurons be marked clean once they are written?</param>
/// <returns>
/// Returns with S_OK when the brain was written, E_POINTER when no stream was supplied,
/// or the first failure code a neuron reported.
/// </returns>
HRESULT BrainClass::Save(IStream * stream, BOOL cleardirty)
{
	if (stream == NULL) {
		return(E_POINTER);
	}

	int count = Neurons.Count();

	HRESULT result = stream->Write(&count, sizeof(count), NULL);
	if (SUCCEEDED(result)) {
		for (int i = 0; i < count; i++) {
			result = Neurons[i]->Save(stream, cleardirty);
			if (FAILED(result)) {
				return(result);
			}
		}

		result = S_OK;
	}
	return(result);
}


/// <summary>
/// Reads this brain back from the save game stream.
/// A neuron is created and read back for each one the stream says was written, and each
/// is handed to the brain as it arrives.
/// </summary>
/// <returns>
/// Returns with S_OK when the brain was read, E_POINTER when no stream was supplied,
/// E_OUTOFMEMORY when a neuron could not be created, or the stream's own failure code.
/// </returns>
HRESULT BrainClass::Load(IStream * stream)
{
	if (stream == NULL) {
		return(E_POINTER);
	}

	int count = 0;

	HRESULT result = stream->Read(&count, sizeof(count), NULL);
	if (SUCCEEDED(result)) {
		for (int i = 0; i < count; i++) {
			NeuronClass * neuron = new NeuronClass;
			if (neuron == NULL) {
				return(E_OUTOFMEMORY);
			}

			result = neuron->Load(stream);
			if (FAILED(result)) {
				return(result);
			}

			Add_Neuron(neuron);
		}

		result = S_OK;
	}
	return(result);
}
