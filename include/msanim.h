/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 * Copyright 2026 ra2-decomp contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "rect.h"
#include "rgb.h"
#include "stimer.h"
#include "timer.h"
#include "vector.h"
#include "widewstring.h"

#include "draw.hh"

class ConvertClass;
class MSFont;
class MSAnim;
class Surface;
class BSurface;
class BinkHandle;
class ShapeSet;
class Wstring;
class SHPFrame;
struct VQHandle;

typedef DynamicVectorClass<MSAnim *> MS_ANIM_LIST;
typedef VectorCursor<MSAnim *, MS_ANIM_LIST> MS_ANIM_ITER;

/*
**	RA2: the menu anims hold their shapes as the raw shape file data rather than as a ShapeSet,
**	and fetch a frame's bounds through accessors of their own. A frame that does not exist has
**	the bounds of NullFrameRect.
*/
class SHPStruct
{
	public:
		int Get_Width(void) const {return(Width);}
		int Get_Height(void) const {return(Height);}
		int Get_Count(void) const {return(Count);}

		bool Is_Frame_Valid(int frame) const {return(unsigned(frame) < unsigned(Count));}
		inline SHPFrame const * Fetch_Frame_Pointer(int frame) const;
		inline Rect GetFrameBounds(int frame) const;

		short Flags;
		short Width;
		short Height;
		short Count;
};

class SHPFrame
{
	public:
		short Left;
		short Top;
		short Width;
		short Height;
		unsigned char Flags;
		unsigned char : 8;
		unsigned char : 8;
		unsigned char : 8;
		RGBStruct Color;
		unsigned int Reserved;
		int Offset;
};

extern Rect NullFrameRect;

inline SHPFrame const * SHPStruct::Fetch_Frame_Pointer(int frame) const
{
	if (Is_Frame_Valid(frame)) {
		return(&((SHPFrame const *)(this + 1))[frame]);
	}
	return(NULL);
}

inline Rect SHPStruct::GetFrameBounds(int frame) const
{
	SHPFrame const * data = Fetch_Frame_Pointer(frame);
	if (data != NULL) {
		return(Rect(data->Left, data->Top, data->Width, data->Height));
	}
	return(NullFrameRect);
}


class MSAnim
{
	public:
		MSAnim(int x=0, int y=0, bool active=true) : XPos(x), YPos(y), Active(active) {}
		virtual ~MSAnim(void);

		/// Toggles the active state of the anim
		virtual void Set_Active(bool active) { Active=active; }

		/// Pauses or resumes the anim
		virtual void Pause(void) { Timer.Stop(); }
		virtual void Resume(void) { Timer.Start(); }

		/// Process anim logic and draw the anim
		/// Returns true if the anim should be deleted
		virtual bool Advance(Surface * surface, Rect & rect) = 0;

		/// Draws the anim in its current state
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) = 0;

		/// Returns the anim's bounding rect
		virtual Rect Get_Rect(void) const = 0;

		/// Returns if the anim's done drawing itself
		virtual bool Has_Finished(void) const { return(false); }

		/// Restores the anim's image to the surface
		virtual void Restore(Rect const & rect) { }

	protected:
		/*
		 * These are the screen coordinates, expressed in pixels, that the anim draws itself
		 * at. The printing anims adjust them as the text is fitted to its rectangle.
		 */
		int XPos;
		int YPos;

		/*
		 * If this anim is taking part in the display, then this flag will be true. An
		 * inactive anim neither advances nor draws itself, but is otherwise left alone.
		 */
		bool Active;

		/*
		 * This is the countdown between one step of the animation and the next. An anim only
		 * does work on the pass where this reaches zero, and then reloads it with its rate.
		 */
		CDTimerClass<SystemTimerClass> Timer;
};


class MSShapeAnim : public MSAnim
{
	public:
		MSShapeAnim(Wstring const & name, int x, int y, ConvertClass * drawer, int rate, bool loop=true, ShapeFlags_Type flags=SHAPE_NORMAL); // RA2: Wstring name
		virtual ~MSShapeAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;

		void Set_Frame(unsigned frame);
		void Set_Start_Frame(unsigned frame);
		void Set_Stop_Frame(unsigned frame);

	protected:
		/*
		 * Pointer to the shape data the animation frames are drawn from.
		 */
		SHPStruct * Shape; // RA2

		/*
		 * Pointer to the drawer that supplies the palette and blitters the shape is rendered
		 * with. The anim does not own it -- the screen that created it does.
		 */
		ConvertClass * Drawer;

		/*
		 * This is the delay between animation frames, expressed in game frames. It is what
		 * the anim's timer is reloaded with each time a frame is drawn.
		 */
		int Rate;

		/*
		 * This is the shape frame currently on display. MSOverlayAnim uses it as the fade
		 * stage instead, since it only ever shows the one frame.
		 */
		unsigned CurFrame;

		/*
		 * This is the frame a looping anim comes back to once it has run past its stop
		 * frame. It starts out at the first frame of the shape.
		 */
		unsigned StartFrame;

		/*
		 * This is the last frame of the animation -- past it the anim either loops back to
		 * its start frame or reports itself finished. It starts out at the shape's last frame.
		 */
		unsigned StopFrame;

		/*
		 * If the animation is to start over when it runs off the end, then this flag will be
		 * true. Otherwise the anim reports itself finished and is deleted.
		 */
		bool Loop;

		/*
		 * These are the drawing flags the shape is rendered with. SHAPE_CENTER is honored by
		 * the anim itself as well, since it has to bias its own bounding rect to match.
		 */
		ShapeFlags_Type ShapeFlags;

		/*
		 * If this anim loaded the shape itself, then this flag will be true. A shape borrowed
		 * from the mix file catalog is left alone; only one loaded here is freed again.
		 */
		bool AllocLoaded;
};


class MSFadeAnim : public MSShapeAnim
{
	public:
		MSFadeAnim(Wstring const & name, int x, int y, ConvertClass * drawer, int rate, ShapeFlags_Type flags, MS_ANIM_LIST * vector); // RA2: Wstring name
		virtual ~MSFadeAnim(void) override {}

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;

	protected:
		unsigned FadeStage; // RA2: +0x40, index into the fade shape flags, 4 once faded

		/*
		 * Pointer to the list of anims this one shares the screen with. The shape is drawn
		 * see-through, so the siblings behind it must repair themselves over it every pass.
		 */
		MS_ANIM_LIST * Anims;
};


class MSBitPrintAnim : public MSAnim
{
	public:
		MSBitPrintAnim(WideWstring const & text, Rect const & rect, bool flag);
		virtual ~MSBitPrintAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;
		virtual bool Has_Finished(void) const override;

		void Set_Flag(bool flag);

	protected:

		WideWstring Text;

		Rect Area;

		bool IsFinished; // RA2: +0x30, YR: FlagA; Has_Finished returns it, Set_Flag sets it

		bool IsCentered; // RA2: +0x31, YR: FlagB; centres each line (BitText flags 1)
};


/*
**	RA2: an inactive anim covering the whole 640 x 480 screen. Nothing in the game
**	constructs one.
*/
class MSFrameAnim : public MSAnim
{
	public:
		MSFrameAnim(void) : MSAnim(0, 0, false) {}
		virtual ~MSFrameAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;
};


/*
**	RA2: plays a Bink movie into an area of the screen, redrawing the other anims of its
**	list over the movie as it advances.
*/
class MSBinkAnim : public MSAnim
{
	public:
		MSBinkAnim(Wstring const & name, Surface * surface, MS_ANIM_LIST * vector, Rect const & rect, bool held);
		virtual ~MSBinkAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;
		virtual bool Has_Finished(void) const override;

	protected:
		BinkHandle * Player; // RA2: +0x1C, made from the name and Surface; NULL for an empty name
		Surface * Surface; // RA2: +0x20, what the movie is drawn to
		Rect Area; // RA2: +0x24
		MS_ANIM_LIST * Anims; // RA2: +0x34, the anims redrawn over the movie
		bool IsHeld; // RA2: +0x38, the anim is not finished when the movie ends
};


class MSOverlayAnim : public MSFadeAnim
{
	public:
		MSOverlayAnim(Wstring const & name, int x, int y, ConvertClass * drawer, int rate, MS_ANIM_LIST * vector, bool persistent=false, unsigned frame=0, bool fade=true); // RA2: Wstring name, fade flag
		virtual ~MSOverlayAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual bool Has_Finished(void) const override;
		virtual void Restore(Rect const & rect) override;

		void Restart(void);
		void Set_Frame(int frame);

	protected:
		/*
		 * If the anim is to stay on the list once its fade has completed, then this flag
		 * will be true. Otherwise it asks to be deleted as soon as the frame is stamped down.
		 */
		bool Persistent; // RA2: the frame is the shape anim's current frame
};


class MSVQAnim : public MSAnim
{
	public:
		MSVQAnim(Wstring const & name, Surface * surface, MS_ANIM_LIST * vector, bool persistent=false); // RA2: Wstring name
		MSVQAnim(Wstring const & name, Surface * surface, MS_ANIM_LIST * vector, Rect const & rect, bool persistent=false); // RA2
		virtual ~MSVQAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;
		virtual bool Has_Finished(void) const override;
		virtual void Restore(Rect const & rect) override;

	protected:
		/*
		 * Pointer to the movie being played. If the movie could not be created, then this is
		 * NULL and the anim falls back on its still picture alone.
		 */
		VQHandle * Movie;

		/*
		 * Pointer to the surface the movie is played onto, and the one the still picture is
		 * committed to once the movie has ended.
		 */
		Surface * TargetSurface;

		/*
		 * Pointer to the list of anims this one shares the screen with. Every frame the movie
		 * puts up covers them, so they are asked to repair themselves over it.
		 */
		MS_ANIM_LIST * Anims;

		/*
		 * Pointer to the still picture loaded from the PCX file sharing the movie's name. It
		 * takes the movie's place when it ends, so the screen is not left blank.
		 */
		Surface * Background;

		/*
		 * If the anim is to stay on the list after the movie has ended, then this flag will
		 * be true. Otherwise it asks to be deleted as the last frame goes by.
		 */
		bool Persistent;

		/*
		 * If the movie has run to its end, then this flag will be true. A finished movie
		 * draws nothing more, and it is the still picture that gets restored from then on.
		 */
		bool Done;
};


class MSPrintAnim : public MSAnim
{
	public:
		MSPrintAnim(WideWstring const & text, int x, int y, MSFont * font, Rect const & rect=RECT_NONE, int start_delay=0, int print_delay=4, bool fade_effect=true, bool center=false); // RA2: wide text
		virtual ~MSPrintAnim(void) override {}

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;
		virtual bool Has_Finished(void) const override;

		void Set_Transient(bool transient);
		static int Word_Wrap(WideWstring & string, MSFont * font, int line_width); // RA2: wide text
		static int Paginate(WideWstring & string, MSFont * font, int page_height); // RA2: wide text

	protected:
		void Set_Dimensions(Rect const & rect);
		int Get_Line_Width(WideWstring const & str) const; // RA2: wide text
		int Get_Printed_Char_Count(void) const { return(PrintedCharCount); }

	protected:
		/*
		 * This is the text being typed out, held in a copy of its own.
		 */
		WideWstring Text; // RA2

		/*
		 * This is the index of the first character of the topmost line still shown. It is
		 * advanced by a line when the text runs past the bottom of its rectangle.
		 */
		unsigned LineStart;

		/*
		 * This is the number of characters typed out so far. The characters within two of it
		 * are drawn part faded, which trails the teletype effect behind the newest one.
		 * MSWordAnim reuses it as the fade stage of the line it is bringing up.
		 */
		unsigned PrintedCharCount;

		/*
		 * This is the number of passes made since the end of the string was reached. The anim
		 * waits out a few of them before finishing, so the trailing fade has time to run out.
		 */
		unsigned CompletionCount;

		/*
		 * This is the delay between one character and the next, expressed in game frames. It
		 * is what the anim's timer is reloaded with each pass.
		 */
		int PrintDelay;

		/*
		 * If the characters are to fade in as they are printed, then this flag will be true.
		 * The text area is restored from the backdrop every pass so the fade can be redrawn.
		 */
		bool FadeEffect;

		/*
		 * Pointer to the font the text is printed and measured with.
		 */
		MSFont * Font;

		/*
		 * This is the area the text is fitted to and clipped within, in screen coordinates.
		 * Its width is also what a centered line is centered against.
		 */
		Rect Area;

		/*
		 * If the anim is to report itself finished once its string is printed, then this flag
		 * will be true. Advance leaves it alone, so its owner decides when to retire it.
		 */
		bool Transient;

		/*
		 * If every line of the text is to be centered within the rectangle, then this flag
		 * will be true. The indent is worked out afresh for each line as it is reached.
		 */
		bool Centered;

		/*
		 * This is the indent, expressed in pixels, that centers the line being printed within
		 * the rectangle. It is only meaningful while the text is centered.
		 */
		int Offset;
};


class MSWordAnim : public MSPrintAnim
{
	public:
		MSWordAnim(WideWstring const & text, int x, int y, MSFont * font, Rect const & rect=RECT_NONE, int start_delay=0, int print_delay=4, bool fade_effect=true, bool center=false) :
			MSPrintAnim(text, x, y, font, rect, start_delay, print_delay, fade_effect, center), LineYPos(YPos)
		{
			LineRect.Set(0, 0, 0, 0);
		}

		virtual ~MSWordAnim(void) override {}

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;

	protected:
		/*
		 * This is the screen row the line being brought up is printed at. It starts level
		 * with the anim itself and steps down a font height as each line is completed.
		 */
		int LineYPos;

		/*
		 * This is the area the line currently fading in occupies. It is restored from the
		 * backdrop on each pass of the fade, and cleared once the line has fully arrived.
		 */
		Rect LineRect;
};


/*
**	RA2: the text a button anim prints, laid out to the button's width.
*/
class MSTextLayout
{
	public:
		MSTextLayout(char const * text, int width);
		~MSTextLayout(void);

		void Set_Position(int x, int y);
		bool Is_Complete(void) const;
		bool Is_Dirty(void) const;

	private:
		char Data[0x30]; // RA2: 48 bytes
};


class MSButtonAnim : public MSAnim
{
	public:
		MSButtonAnim(Wstring const & text, int width, MS_ANIM_LIST * vector, Rect const & rect, bool persistent); // RA2
		virtual ~MSButtonAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;
		virtual bool Has_Finished(void) const override; // RA2
		virtual void Restore(Rect const & rect) override;

	protected:
		/*
		 * This is the caption laid out for printing. If NULL, then the button has no text.
		 */
		MSTextLayout * Text; // RA2

		/*
		 * This is the width, in pixels, the caption is laid out to.
		 */
		int Width; // RA2

		/*
		 * This is the screen area the button occupies.
		 */
		Rect Area;

		/*
		 * This is the list of anims the button belongs to.
		 */
		MS_ANIM_LIST * Anims; // RA2

		/*
		 * If the button is to stay on the list once its text is complete, then this flag will
		 * be true.
		 */
		bool Persistent; // RA2
};


class MSPCXAnim : public MSAnim
{
	public:
		MSPCXAnim(Wstring const & name, MS_ANIM_LIST * vector, bool transient=true); // RA2: Wstring name
		MSPCXAnim(Wstring const & name, MS_ANIM_LIST * vector, Point2D const & position, bool transient=true); // RA2: Wstring name
		virtual ~MSPCXAnim(void) override;

		virtual bool Advance(Surface * surface, Rect & rect) override;
		virtual void Redraw(Surface * surface, Rect const * rect=NULL) override;
		virtual Rect Get_Rect(void) const override;
		virtual bool Has_Finished(void) const override;
		virtual void Restore(Rect const & rect) override;

	public:
		/*
		 * Pointer to the surface the picture is committed to. This is the backdrop the other
		 * anims restore themselves from, so the picture becomes part of the scene.
		 */
		Surface * TargetSurface;

		/*
		 * Pointer to the list of anims this one shares the screen with. They are asked to
		 * repair themselves over the picture once it has been put up.
		 */
		MS_ANIM_LIST * Anims;

		/*
		 * Pointer to the picture read out of the PCX file. The anim owns it and frees it
		 * again on destruction. If the file could not be found, then this is NULL.
		 */
		Surface * Image;

		/*
		 * If the anim is to be retired by its owner rather than by itself, then this flag
		 * will be true. Otherwise it asks to be deleted as soon as the picture is up.
		 */
		bool Transient;

		/*
		 * If the picture has been put up, then this flag will be true. It stops the image
		 * being drawn a second time, since one pass is all it takes.
		 */
		bool Drawn;

		/*
		 * This is the area of the screen the picture is drawn into. The picture is centered
		 * on the surface unless an explicit position was supplied.
		 */
		Rect Area;
};


ConvertClass * Create_Drawer(Wstring const & palette_name); // RA2: Wstring name
