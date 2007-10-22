# -*- perl -*-

use Tk::MMutil;
use Config;
use File::Spec;
use File::Basename qw(dirname);

# Find path to existing pTk include files
my $ptkPath = dirname(Tk::MMutil::findINC(File::Spec->catfile(qw(Tk pTk Lang.h))));

my $canvasSo = Tk::MMutil::findINC(File::Spec->catfile(qw(auto Tk Canvas), 'Canvas.'.$Config{so}));


Tk::MMutil::TkExtMakefile(
			  VERSION_FROM => "Point.pm",
			  XS_VERSION => "0.04", # should match VERSION_FROM
			  NAME => 'Tk::Canvas::Point',
			  OBJECT => '$(O_FILES) ',
			  MYEXTLIB => $canvasSo,
			  INC    => "-I$ptkPath",
			  $^O eq 'MSWin32' ? () : ('LIBS'        => ["-lm"]),
			  EXTRA_META => <<'EOF',
configure_requires:
  Tk::MMutil: 0
  File::Spec: 0
EOF
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
