#!/usr/bin/perl -w
# -*- perl -*-

#
# $Id: tk-canvas-point.t,v 1.4 2002/07/24 15:24:10 eserte Exp $
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
$c->bind("all", "<1>" => sub {
	     my($c) = @_;
	     my(@tags) = $c->gettags("current");
	     warn "Tags of current item: @tags\n";
	     my(@coords) = $c->coords("current");
	     warn "Coords of current item: @coords\n";
	 });
my $status = "";
$c->bind("all", "<Enter>" => sub {
	     my($c) = @_;
	     $status = ($c->gettags("current"))[0];
	 });
$c->bind("all", "<Leave>" => sub {
	     my($c) = @_;
	     $status = "";
	 });
my @p;
$mw->Button(-text => "Create points",
	    -command => sub {
		for(1..100) {
		    my $width = rand(20);
		    push @p, $c->create('point',
					rand($c->width),rand($c->height),
					-width => $width,
					-activefill => "white",
					-activewidth => $width+2,
					-fill => [qw(blue red green yellow black white)]->[rand(5)],
					-tags => "tag".int(rand(100)),
				       );
		}
	    })->pack;
$mw->Button(-text => "Delete last point",
	    -command => sub { $c->delete(pop @p) if @p; },
	   )->pack;
$mw->Label(-textvariable => \$status)->pack;

MainLoop;

__END__
