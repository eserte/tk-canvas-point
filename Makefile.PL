# -*- perl -*-

# Find path to existing pTk include files
my $ptkPath = Tk::MMutil::findINC('Tk/pTk/Lang.h');
$ptkPath =~ s/\/Lang.h$//g;

use Tk::MMutil;
Tk::MMutil::TkExtMakefile(
			  NAME => 'Tk::Canvas::Point',
			  OBJECT => '$(O_FILES)',
			  INC    => "-I$ptkPath",
			  $^O eq 'MSWin32' ? () : ('LIBS'        => ["-lm"]),
			 );
