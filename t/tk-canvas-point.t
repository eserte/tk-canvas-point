#!/usr/bin/perl -w
# -*- perl -*-

#
# $Id: tk-canvas-point.t,v 1.3 2002/07/24 14:44:10 eserte Exp $
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
my $c = $mw->Canvas->pack(-fill => "both", -expand => 1);
my @p;
$mw->Button(-text => "Create points",
	    -command => sub {
		for(1..100) {
		    push @p, $c->create('point',rand($c->width),rand($c->height),-width => rand(20), -fill => [qw(blue red green yellow black white)]->[rand(5)]);
		}
	    })->pack;
$mw->Button(-text => "Delete last point",
	    -command => sub { $c->delete(pop @p) if @p; },
	   )->pack;
MainLoop;

__END__
