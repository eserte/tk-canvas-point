#!/usr/bin/perl -w
# -*- perl -*-

#
# $Id: tk-canvas-point.t,v 1.2 2002/07/24 14:35:02 eserte Exp $
# Author: Slaven Rezic
#
# Copyright (C) 2002 Slaven Rezic. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.
#
# Mail: slaven.rezic@berlin.de
# WWW:  http://www.rezic.de/eserte/
#

use Tk;
use Tk::Canvas::Point;
use strict;

my $mw = MainWindow->new;
my $c = $mw->Canvas->pack;
my $p = $c->create('point',100,100,-width => 20, -fill => "blue");
$mw->Button(-text => "Delete point",
	    -command => sub { $c->delete($p) },
	   )->pack;
MainLoop;

__END__
