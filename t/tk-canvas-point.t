#!/usr/bin/perl -w
# -*- perl -*-

#
# $Id: tk-canvas-point.t,v 1.6 2004/08/08 16:36:43 eserte Exp $
# Author: Slaven Rezic
#
# Copyright (C) 2002 Slaven Rezic. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.
#
# Mail: slaven@rezic.de
# WWW:  http://www.rezic.de/eserte/
#

use Test::More qw(no_plan);
use Tk;
use Tk::Canvas::Point;
use strict;

$ENV{BATCH} = 1 unless defined $ENV{BATCH};

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
if ($ENV{BATCH}) {
    $mw->update;
    create_points();
    $mw->update;
    for (1..10) {
	delete_last_point();
	$mw->update;
    }
    postscript(0);
    $mw->update;
} else {
    my $f = $mw->Frame->pack;
    $mw->Button(-text => "Create points",
		-command => sub { create_points() }
	       )->pack(-side => "left");
    $mw->Button(-text => "Delete last point",
		-command => sub { delete_last_point() },
	       )->pack(-side => "left");
    $mw->Button(-text => "PS",
		-command => sub { postscript(1) })->pack(-side => "left");
}
$mw->Label(-textvariable => \$status)->pack;

MainLoop if !$ENV{BATCH};

ok(1);

sub create_points {
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
}

sub delete_last_point {
    $c->delete(pop @p) if @p;
}

sub postscript {
    my $display = shift;
    my $f = "/tmp/test.$$.ps";
    $c->postscript(-file => $f);
    ok(-f $f);
    open(F, $f) or die $!;
    my($firstline) = <F>;
    close F;
    like($firstline, qr/%!PS-Adobe-\d/);
    system("gv $f &") if $display;
}

__END__
