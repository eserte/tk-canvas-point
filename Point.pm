# -*- perl -*-

#
# $Id: Point.pm,v 1.2 2002/07/24 15:24:24 eserte Exp $
# Author: Slaven Rezic
#
# Copyright (C) 2002 Slaven Rezic. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.
#
# Mail: slaven.rezic@berlin.de
# WWW:  http://www.rezic.de/eserte/
#

package Tk::Canvas::Point;
use Tk;
use Tk::Canvas;
use strict;
use vars qw($VERSION);
#$VERSION = sprintf("%d.%02d", q$Revision: 1.2 $ =~ /(\d+)\.(\d+)/);
$VERSION = $Tk::VERSION;

use base qw(DynaLoader);

__PACKAGE__->bootstrap;

1;

__END__
