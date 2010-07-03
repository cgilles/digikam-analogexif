#!/bin/sh

if [ $# -lt 1 ]
then
	echo "Usage:\n\t$0 <ReleaseName>"
	exit
fi

# write XML header
echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Releases>" > version.xml

# extract release information from current-version.xml
# NB: I'm baad at unix command-line

cat current-version.xml | awk '
BEGIN {
	found = 0;
	section = "";
}
{
	if($0 ~ "<Release>")
	{
		if(found == 0)
		{
			section = $0;
		}
	}
	else
	{
		section = section "\n" $0
		if($0 ~ "<OS>'$1'</OS>")
		{
			found = 1;
		}
		
		if($0 ~ "</Release>")
		{
			if(found == 1)
			{
				print section;
			}
		}
	}
}' >> version.xml

# close <Releases> section
echo "</Releases>" >> version.xml

