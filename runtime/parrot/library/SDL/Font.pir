
# $Id$

=head1 NAME

SDL::Font - Parrot class representing fonts in Parrot SDL

=head1 SYNOPSIS

	# load this library
	load_bytecode 'library/SDL/Font.pir'

	# set the font's arguments
	.local pmc font_args
	font_args                 = new .Hash
	font_args[ 'font_file'  ] = 'myfont.ttf'
	font_args[ 'point_size' ] = 48

	# create a new SDL::Font object
	.local pmc font
	.local int font_type

	find_type font_type, 'SDL::Font'
	font = new font_type, font_args

	# draw text to the screen
	#	presuming you have an SDL::Surface, SDL::Color, and SDL::Rect here...
	font.'draw'( 'some text', font_color, destination_surface, dest_rect )

	# or render it to a surface to use later
	font.'render_text'( 'some text', font_color )

=head1 DESCRIPTION

A SDL::Font object represents a TrueType font in SDL.  You can use this to draw fonts to any L<SDL::Surface>.

=head1 METHODS

All SDL::Font objects have the following methods:

=over

=cut

.namespace [ 'SDL::Font' ]

.sub _sdl_init :load
	.local pmc init_ttf
	init_ttf = find_global 'SDL', '_init_ttf'
	init_ttf()

	.local pmc   font_class

	newclass     font_class, 'SDL::Font'
	addattribute font_class, 'font'
	addattribute font_class, 'size'

	.return()
.end

=item init( font_args )

Given a C<Hash> containing arguments, set the attributes of this font.  The
keys of this hash are C<font_file> and C<point_size>, two strings containing
the path to a TrueType font to load and the size of the font when drawn, in
pixels.

The name of this method may change.

=cut

.sub _BUILD :method
	.param pmc    args

	.local string font_name
	.local int    font_size

	font_name = args[ 'font_file'  ]
	font_size = args[ 'point_size' ]

	.local pmc OpenFont
	OpenFont = find_global 'SDL::NCI::TTF', 'OpenFont'

	.local pmc font
	font = OpenFont( font_name, font_size )

	.local int offset
	classoffset offset,   self, 'SDL::Font'
	setattribute  self, offset, font
	inc offset

	.local pmc size_value
	size_value = new Integer
	size_value = font_size
	setattribute self, offset, size_value

.end

=item draw( text_string, text_color, dest_surface, dest_rect )

Given a string of text to draw, an C<SDL::Color> object representing the color
of the text to draw, a C<SDL::Surface> to which to draw, and a C<SDL::Rect>
representing the placement of the text within the surface, draws some text.

Whew.

=cut

.sub draw :method
	.param string text
	.param pmc    color_pmc
	.param pmc    screen
	.param pmc    dest_rect

	.local pmc font_surface

	font_surface = self.'render_text'( text, color_pmc )

	.local int w
	.local int h
	w = font_surface.'width'()
	h = font_surface.'height'()

	.local int rect_type
	.local pmc rect

	.local pmc rect_args
	rect_args        = new .Hash
	rect_args[ 'x' ] = 0
	rect_args[ 'y' ] = 0

	find_type rect_type, 'SDL::Rect'
	rect = new rect_type, rect_args

	rect.'height'( h )
	rect.'width'( w )
	rect.'x'( 0 )
	rect.'y'( 0 )

	dest_rect.'height'( h )
	dest_rect.'width'( w )

	screen.'blit'( font_surface, rect, dest_rect )

.end

=item render_text( text_string, text_color )

Renders a string of text of the given C<SDL::Color>.  This returns a new
C<SDL::Surface> containing the rendered font.

=cut

.sub render_text :method
	.param string text
	.param pmc    color_pmc

	.local pmc font
	font = self.'font'()

	.local int surface_type
	find_type surface_type, 'SDL::Surface'

	.local pmc font_surface
	font_surface = new surface_type

	.local pmc RenderText_Solid
	find_global RenderText_Solid, 'SDL::NCI::TTF', 'RenderText_Solid'

	.local int color
	color = color_pmc.'color'()

	.local pmc font_surface_struct
	font_surface_struct = RenderText_Solid( font, text, color )

	font_surface.'wrap_surface'( font_surface_struct )

	.return( font_surface )
.end

=item font()

Returns the underlying C<SDL_Font> structure this object wraps.  You should
never need to call this directly unless you're calling SDL functions directly,
in which case why not send me a patch?

=cut

.sub font :method
	.local int offset
	classoffset offset, self, 'SDL::Font'

	.local pmc font
	getattribute font, self, offset

	.return( font )
.end

=item point_size( [ new_size ] )

Gets or sets the point size associated with this font object.  The single
argument is an integer and is optional.

=cut

.sub point_size :method
	.param int new_size

	.local int size
	.local int param_count
	.local int offset

	param_count = I1
	classoffset offset, self, 'SDL::Font'
	inc offset

	.local pmc size_value
	size_value = new Integer

	if param_count == 0 goto getter

	size_value = new_size
	setattribute self, offset, size_value

getter:
	getattribute size_value, self, offset
	size = size_value

	.return( size )
.end

=back

=head1 AUTHOR

Written and maintained by chromatic, E<lt>chromatic at wgz dot orgE<gt>.
Please send patches, feedback, and suggestions to the Perl 6 Internals mailing
list.

=head1 COPYRIGHT

Copyright (C) 2004-2006, The Perl Foundation.

=cut
