# -*- perl -*-

use Tk::MMutil;
use Config;

# Find path to existing pTk include files
my $ptkPath = Tk::MMutil::findINC('Tk/pTk/Lang.h');
$ptkPath =~ s/\/Lang.h$//g;

my $canvasSo = Tk::MMutil::findINC('auto/Tk/Canvas/Canvas.'.$Config{so});


Tk::MMutil::TkExtMakefile(
			  VERSION => "0.02",
			  XS_VERSION => "0.02",
			  NAME => 'Tk::Canvas::Point',
			  OBJECT => '$(O_FILES) ',
			  MYEXTLIB => $canvasSo,
			  INC    => "-I$ptkPath",
			  $^O eq 'MSWin32' ? () : ('LIBS'        => ["-lm"]),
			 );

sub MY::postamble {
    my $postamble = '
';

    if (defined $ENV{USER} && $ENV{USER} eq 'eserte' && $^O =~ /bsd/i && -f "../../perl.release.mk") {
	$postamble .= <<'EOF';

.include "../../perl.release.mk"
.include "../../perl.cvs.mk"

EOF
    }

    $postamble;
}
