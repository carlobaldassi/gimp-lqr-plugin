; GIMP LiquidRescale Plug-in
; Copyright (C) 2007-2009 Carlo Baldassi (the "Author") <carlobaldassi@gmail.com>.
; All Rights Reserved.
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the Licence, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, see <http://www.gnu.org.licences/>.
;

(define
  (batch-gimp-lqr			; run with default arguments, except for:
	filename			; (STRING) filname
	width				; (INT) new width
	height				; (INT) new height
	layer_name			; (STRING) layer_name (empty for active layer)
	pres_layer_name			; (STRING) preservation layer name (empty for none)
	disc_layer_name			; (STRING) discard layer name (empty for none)
	)
  (let*
    (
      (image (car (gimp-file-load RUN-NONINTERACTIVE filename filename)))
      (drawable (car (gimp-image-get-active-layer image)))
    )
    (plug-in-lqr
       		RUN-NONINTERACTIVE	;(INT "run-mode" "Interactive, non-interactive")
		image			;(IMAGE "image" "Input image")
		drawable		;(DRAWABLE "drawable" "Input drawable")
		width			;(INT "width" "Final width")
		height			;(INT "height" "Final height")
		0			;(INT "pres-layer" "Layer that marks preserved areas")
		1000			;(INT "pres-coeff" "Preservation coefficient (for interactive mode only)")
		0			;(INT "disc-layer" "Layer that marks areas to discard")
		1000			;(INT "disc-coeff" "Discard coefficient")
		0			;(FLOAT "rigidity" "Rigidity coefficient")
		0			;(INT "rigmask-layer" "Layer used as rigidity mask")
		1			;(INT "delta-x" "max displacement of seams")
		150			;(FLOAT "enl-step" "enlargment step (ratio)")
		1			;(INT "resize-aux-layers" "Whether to resize auxiliary layers")
		1			;(INT "resize-canvas" "Whether to resize canvas")
		0			;(INT "new-layer" "Whether to output on a new layer")
		0			;(INT "seams" "Whether to output the seam map")
		3			;(INT "grad-func" "Gradient function to use")
		0			;(INT "res-order" "Resize order")
		0			;(INT "mask-behavior" "What to do with masks")
		0			;(INT "scaleback" "Whether to scale back when done")
		0			;(INT "scaleback-mode" "Scale back mode")
		1			;(INT "no-disc-on-enlarge" "Ignore discard layer upon enlargement")
		pres_layer_name		;(STRING "pres-layer-name" "Preservation layer name (overcomes pres-layer)")
		disc_layer_name		;(STRING "disc-layer-name" "Discard layer name (overcomes disc-layer)")
		""			;(STRING "rigmask-layer-name" "Rigidity mask layer name (overcomes rigmask-layer)"))
		layer_name		;(STRING "selected-layer-name" "Selected layer name (overcomes the active layer)"))
                )
    (set! drawable (car (gimp-image-get-active-layer image)))
    (gimp-file-save RUN-NONINTERACTIVE image drawable filename filename)
    (gimp-image-delete image)
  )
)

(define
  (batch-gimp-lqr-full			; run with full arguments
	filename			; (STRING) filname
	outfilename			; (STRING) output filename
	width				; (INT) new width
	height				; (INT) new height
	layer_name			; (STRING) layer name (empty for active layer)
	pres_layer_name			; (STRING) preservation layer name (empty for none)
	pres_coeff			; (INT) preservation coefficient (default=1000)
	disc_layer_name			; (STRING) discard layer name (empty for none)
	disc_coeff			; (INT) discard coefficient (default=1000)
	rigidity			; (FLOAT) overall rigidity coefficient
	rigmask_layer_name		; (STRING) rigidity mask layer name (empty for none)
	delta_x				; (INT) max displacement along a seam (default=1)
	enlargement_step		; (FLOAT) enlargement step percentage (default=150)
	resize_aux_layers		; (INT) whether to resize auxiliary layers (0=False [1=True])
	resize_canvas			; (INT) whether to resize canvas (0=False [1=True])
	new_layer			; (INT) whether to output on a new layer ([0=False] 1=True)
	output_seams			; (INT) whether to output the seam map(s) ([0=False] 1=True)
	gradient_function		; (INT) gradient function to use (0=Norm 2=SumAbs [3=xAbs] 5=Null)
	resize_order			; (INT) resize order ([0=HorizontalFirst] 1=VerticalFirst)
	mask_behaviour			; (INT) what to do when a mask is found ([0=Apply] 1=Discard)
	scaleback			; (INT) whether to scale back when done ([0=False] 1=True)
	scaleback_mode			; (INT) scale back mode ([0=LqR] 1=Standard 2=StdW 3=StdH)
	no_disc_on_enlarge		; (INT) ignore discard layer upon enlargement (0=False [1=True])
	)
  (let*
    (
      (image (car (gimp-file-load RUN-NONINTERACTIVE filename filename)))
      (drawable (car (gimp-image-get-active-layer image)))
    )
    (plug-in-lqr
       		RUN-NONINTERACTIVE	;(INT "run-mode" "Interactive, non-interactive")
		image			;(IMAGE "image" "Input image")
		drawable		;(DRAWABLE "drawable" "Input drawable")
		width			;(INT "width" "Final width")
		height			;(INT "height" "Final height")
		0			;(INT "pres-layer" "Layer that marks preserved areas")
		pres_coeff		;(INT "pres-coeff" "Preservation coefficient (for interactive mode only)")
		0			;(INT "disc-layer" "Layer that marks areas to discard")
		disc_coeff		;(INT "disc-coeff" "Discard coefficient")
		rigidity		;(FLOAT "rigidity" "Rigidity coefficient")
		0			;(INT "rigmask-layer" "Layer used as rigidity mask")
		delta_x			;(INT "delta-x" "max displacement of seams")
		enlargement_step	;(FLOAT "enl-step" "enlargment step (ratio)")
		resize_aux_layers	;(INT "resize-aux-layers" "Whether to resize auxiliary layers")
		resize_canvas		;(INT "resize-canvas" "Whether to resize canvas")
		new_layer		;(INT "new-layer" "Whether to output on a new layer")
		output_seams		;(INT "seams" "Whether to output the seam map")
		gradient_function	;(INT "grad-func" "Gradient function to use")
		resize_order		;(INT "res-order" "Resize order")
		mask_behaviour		;(INT "mask-behavior" "What to do with masks")
		scaleback		;(INT "scaleback" "Whether to scale back when done")
		scaleback_mode		;(INT "scaleback-mode" "Scale back mode")
		no_disc_on_enlarge	;(INT "no-disc-on-enlarge" "Ignore discard layer upon enlargement")
		pres_layer_name		;(STRING "pres-layer-name" "Preservation layer name (overcomes pres-layer)")
		disc_layer_name		;(STRING "disc-layer-name" "Discard layer name (overcomes disc-layer)")
		rigmask_layer_name	;(STRING "rigmask-layer-name" "Rigidity mask layer name (overcomes rigmask-layer)")
		layer_name		;(STRING "selected-layer-name" "Selected layer name (overcomes the active layer)"))
                )
    (set! drawable (car (gimp-image-get-active-layer image)))
    (gimp-file-save RUN-NONINTERACTIVE image drawable outfilename outfilename)
    (gimp-image-delete image)
  )
)

(define
  (batch-gimp-lqr-full-use-id		; run with full arguments (use layer ID's instead of names)
	filename			; (STRING) filname
	outfilename			; (STRING) output filename
	width				; (INT) new width
	height				; (INT) new height
	layer_ID			; (INT) layer ID (0 for active layer)
	pres_layer_ID			; (STRING) preservation layer ID (0 for none)
	pres_coeff			; (INT) preservation coefficient (default=1000)
	disc_layer_ID			; (INT) discard layer ID (0 for none)
	disc_coeff			; (INT) discard coefficient (default=1000)
	rigidity			; (FLOAT) overall rigidity coefficient
	rigmask_layer_ID		; (INT) rigidity mask layer ID (0 for none)
	delta_x				; (INT) max displacement along a seam (default=1)
	enlargement_step		; (FLOAT) enlargement step percentage (default=150)
	resize_aux_layers		; (INT) whether to resize auxiliary layers (0=False [1=True])
	resize_canvas			; (INT) whether to resize canvas (0=False [1=True])
	new_layer			; (INT) whether to output on a new layer ([0=False] 1=True)
	output_seams			; (INT) whether to output the seam map(s) ([0=False] 1=True)
	gradient_function		; (INT) gradient function to use (0=Norm 2=SumAbs [3=xAbs] 5=Null)
	resize_order			; (INT) resize order ([0=HorizontalFirst] 1=VerticalFirst)
	mask_behaviour			; (INT) what to do when a mask is found ([0=Apply] 1=Discard)
	scaleback			; (INT) whether to scale back when done ([0=False] 1=True)
	scaleback_mode			; (INT) scale back mode ([0=LqR] 1=Standard 2=StdW 3=StdH)
	no_disc_on_enlarge		; (INT) ignore discard layer upon enlargement (0=False [1=True])
	)
  (let*
    (
      (image (car (gimp-file-load RUN-NONINTERACTIVE filename filename)))
    )
    (plug-in-lqr
       		RUN-NONINTERACTIVE	;(INT "run-mode" "Interactive, non-interactive")
		image			;(IMAGE "image" "Input image")
		layer_ID		;(DRAWABLE "drawable" "Input drawable")
		width			;(INT "width" "Final width")
		height			;(INT "height" "Final height")
		pres_layer_ID		;(INT "pres-layer" "Layer that marks preserved areas")
		pres_coeff		;(INT "pres-coeff" "Preservation coefficient (for interactive mode only)")
		disc_layer_ID		;(INT "disc-layer" "Layer that marks areas to discard")
		disc_coeff		;(INT "disc-coeff" "Discard coefficient")
		rigidity		;(FLOAT "rigidity" "Rigidity coefficient")
		rigmask_layer_ID	;(INT "rigmask-layer" "Layer used as rigidity mask")
		delta_x			;(INT "delta-x" "max displacement of seams")
		enlargement_step	;(FLOAT "enl-step" "enlargment step (ratio)")
		resize_aux_layers	;(INT "resize-aux-layers" "Whether to resize auxiliary layers")
		resize_canvas		;(INT "resize-canvas" "Whether to resize canvas")
		new_layer		;(INT "new-layer" "Whether to output on a new layer")
		output_seams		;(INT "seams" "Whether to output the seam map")
		gradient_function	;(INT "grad-func" "Gradient function to use")
		resize_order		;(INT "res-order" "Resize order")
		mask_behaviour		;(INT "mask-behavior" "What to do with masks")
		scaleback		;(INT "scaleback" "Whether to scale back when done")
		scaleback_mode		;(INT "scaleback-mode" "Scale back mode")
		no_disc_on_enlarge	;(INT "no-disc-on-enlarge" "Ignore discard layer upon enlargement")
		""			;(STRING "pres-layer-name" "Preservation layer name (overcomes pres-layer)")
		""			;(STRING "disc-layer-name" "Discard layer name (overcomes disc-layer)")
		""			;(STRING "rigmask-layer-name" "Rigidity mask layer name (overcomes rigmask-layer)")
		""			;(STRING "selected-layer-name" "Selected layer name (overcomes the active layer)"))
                )
    (set! layer_ID (car (gimp-image-get-active-layer image)))
    (gimp-file-save RUN-NONINTERACTIVE image layer_ID outfilename outfilename)
    (gimp-image-delete image)
  )
)

(script-fu-register
          "batch-gimp-lqr"				; func name
          "Batch Liquid Rescale (simplified)"		; menu label
          "Invokes the Liquid Rescale plugin \
	   noninteractively. \
	   This is a simplified version in which most \
	   options take their default values."		; description
          "Carlo Baldassi"				; author
          "copyright 2009, Carlo Baldassi"              ; copyright notice
          "2009"					; date created
          "RGB* GRAY*"					; image type that the script works on
	  SF-STRING	"File name" ""
	  SF-VALUE	"New width [INTEGER]" ""
	  SF-VALUE	"New height [INTEGER]" ""
	  SF-STRING	"Name of the layer to operate onto (empty for active layer)" ""
	  SF-STRING	"Name of the preservation layer (empty for none)" ""
	  SF-STRING	"Name of the discard layer (empty for none)" ""
)

(script-fu-register
          "batch-gimp-lqr-full"				; func name
          "Batch Liquid Rescale"			; menu label
          "Invokes the Liquid Rescale plugin \
	   noninteractively. \
	   This is the full-options version."		; description
          "Carlo Baldassi"				; author
          "copyright 2009, Carlo Baldassi"              ; copyright notice
          "2009"					; date created
          "RGB*, GRAY*"					; image type that the script works on
	  SF-STRING	"Input file name" ""
	  SF-STRING	"Output file name" ""
	  SF-VALUE	"New width [INTEGER]" "0"
	  SF-VALUE	"New height [INTEGER]" "0"
	  SF-STRING	"Name of the layer to operate onto (empty for active layer)" ""
	  SF-STRING	"Name of the preservation layer (empty for none)" ""
	  SF-VALUE	"Preservation strength [INTEGER, default=1000]" "1000"
	  SF-STRING	"Name of the discard layer (empty for none)" ""
	  SF-VALUE	"Discard strength [INTEGER, default=1000]" "1000"
	  SF-VALUE	"Rigidity [FLOAT, default=0.0]" "0.0"
	  SF-STRING	"Name of the rigidity mask layer (empty for none)" ""
	  SF-VALUE	"Max seam step [INTEGER, default=1]" "1"
	  SF-VALUE	"Enlargement step (percentage) [FLOAT, default=150.0]" "150.0"
	  SF-TOGGLE	"Resize aux layer [BOOLEAN, default=TRUE]" TRUE
	  SF-TOGGLE	"Resize canvas [BOOLEAN, default=TRUE]" TRUE
	  SF-TOGGLE	"Output on a new layer [BOOLEAN, default=FALSE]" FALSE
	  SF-TOGGLE	"Output the seam map(s) [BOOLEAN, default=FALSE]" FALSE
	  SF-VALUE	"Gradient function [INTEGER, 0=Norm 2=SumAbs 3=xAbs 5=Null, default=3]" "3"
	  SF-VALUE	"Resize order [INTEGER, 0=HorizontalFirst 1=VerticalFirst, default=0]" "0"
	  SF-VALUE	"Mask behaviour [INTEGER, 0=Apply 1=Discard, default=0]" "0"
	  SF-TOGGLE	"Scale back whan done [BOOLEAN, default=FALSE]" FALSE
	  SF-VALUE	"Scaleback mode [INTEGER, 0=LqR 1=Standard 2=StdW 3=StdH, default=0]" "0"
	  SF-TOGGLE	"Ignore discard layer upon enlargment [BOOLEAN, default=TRUE]" TRUE
)


(script-fu-register
          "batch-gimp-lqr-full-use-id"			; func name
          "Batch Liquid Rescale (alternate)"		; menu label
          "Invokes the Liquid Rescale plugin \
	   noninteractively. \
	   This is an alternate full-options version \
	   in which layed ID's are passed as arguments \
	   instead of layer names."			; description
          "Carlo Baldassi"				; author
          "copyright 2009, Carlo Baldassi"              ; copyright notice
          "2009"					; date created
          "RGB*, GRAY*"					; image type that the script works on
	  SF-STRING	"Input file name" ""
	  SF-STRING	"Output file name" ""
	  SF-VALUE	"New width [INTEGER]" "0"
	  SF-VALUE	"New height [INTEGER]" "0"
	  SF-DRAWABLE	"ID of the layer to operate onto (0 for active layer)" 0
	  SF-DRAWABLE	"ID of the preservation layer (0 for none)" 0
	  SF-VALUE	"Preservation strength [INTEGER, default=1000]" "1000"
	  SF-DRAWABLE	"ID of the discard layer (0 for none)" 0
	  SF-VALUE	"Discard strength [INTEGER, default=1000]" "1000"
	  SF-VALUE	"Rigidity [FLOAT, default=0.0]" "0.0"
	  SF-DRAWABLE	"ID of the rigidity mask layer (0 for none)" 0
	  SF-VALUE	"Max seam step [INTEGER, default=1]" "1"
	  SF-VALUE	"Enlargement step (percentage) [FLOAT, default=150.0]" "150.0"
	  SF-TOGGLE	"Resize aux layer [BOOLEAN, default=TRUE]" TRUE
	  SF-TOGGLE	"Resize canvas [BOOLEAN, default=TRUE]" TRUE
	  SF-TOGGLE	"Output on a new layer [BOOLEAN, default=FALSE]" FALSE
	  SF-TOGGLE	"Output the seam map(s) [BOOLEAN, default=FALSE]" FALSE
	  SF-VALUE	"Gradient function [INTEGER, 0=Norm 2=SumAbs 3=xAbs 5=Null, default=3]" "3"
	  SF-VALUE	"Resize order [INTEGER, 0=HorizontalFirst 1=VerticalFirst, default=0]" "0"
	  SF-VALUE	"Mask behaviour [INTEGER, 0=Apply 1=Discard, default=0]" "0"
	  SF-TOGGLE	"Scale back whan done [BOOLEAN, default=FALSE]" FALSE
	  SF-VALUE	"Scaleback mode [INTEGER, 0=LqR 1=Standard 2=StdW 3=StdH, default=0]" "0"
	  SF-TOGGLE	"Ignore discard layer upon enlargment [BOOLEAN, default=TRUE]" TRUE
)

