#!/usr/bin/perl -w
# -*- perl -*-

#
# $Id: tk-canvas-point.t,v 1.1 2002/07/24 13:09:03 eserte Exp $
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
my $p = $c->createPoint(100,100,-width => 2, -fill => "blue");
$c->delete($p);
MainLoop;

__END__
