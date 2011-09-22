use strict;
use warnings;

fix_bad_proj_files('python\python_module_qgis_analysis.vcproj');
delete_sip_files();
exit(0);

sub fix_bad_proj_files
{
    my $file = shift;
    my @lines = ();
    my $badsip = undef;
    open (IN, "<$file") || die "Can't open prj file for reading: $file";
    while (<IN>)
    {
        push @lines, $_;
        if ($_ =~ /RelativePath\S+sipanalysispart3\.cpp/)
        {
            $badsip = $.; 
        }
    }
    close IN;
    if (defined $badsip)
    {
        if (defined $badsip){
            for (my $i=$badsip-2; $i <= $badsip; $i++)
            {
                $lines[$i] = undef;
            }
        }
        open (OUT, ">$file") || die "Can't open prj file for writing: $file";
        foreach (@lines) {
            next if !defined $_;
            print OUT $_;
        }
        close OUT;
    }
    else
    {
        warn "didn't find bad sip output file\n";
    }
}

sub delete_sip_files
{
    my @files = qw(
        python\core\sipcorepart0.cpp
        python\core\sipcorepart1.cpp
        python\core\sipcorepart2.cpp
        python\core\sipcorepart3.cpp
        python\analysis\sipanalysispart0.cpp
        python\analysis\sipanalysispart1.cpp
        python\analysis\sipanalysispart2.cpp
        python\analysis\sipanalysispart3.cpp
        python\gui\sipguipart0.cpp
        python\gui\sipguipart1.cpp
        python\gui\sipguipart2.cpp
        python\gui\sipguipart3.cpp
    );
    
    foreach my $file (@files)
    {
        if ( !-e $file)
        {
            warn "Can't find file: $file\n";
            next;
        }
        if ( -s $file == 0)
        {
            unlink $file || warn "Can't delete file: $file\n";
        }
    }
}

