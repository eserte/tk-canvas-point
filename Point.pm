# -*- perl -*-

#
# $Id: Point.pm,v 1.4 2004/08/08 16:30:43 eserte Exp $
# Author: Slaven Rezic
#
# Copyright (C) 2002,2004 Slaven Rezic. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.
#
# Mail: slaven@rezic.de
# WWW:  http://www.rezic.de/eserte/
#

package Tk::Canvas::Point;
use Tk;
use Tk::Canvas;
use strict;
use vars qw($VERSION);
$VERSION = "0.02";

use base qw(DynaLoader);

__PACKAGE__->bootstrap;

1;

__END__
