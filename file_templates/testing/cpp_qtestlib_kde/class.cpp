{% load kdev_filters %}
{% include "license_header_cpp.txt" %}


#include "{{ output_file_header }}"


QTEST_GUILESS_MAIN({{ name }});


void {{ name }}::initTestCase()
{
    // Called before the first testfunction is executed
}


void {{ name }}::cleanupTestCase()
{
    // Called after the last testfunction was executed
}


void {{ name }}::init()
{
    // Called before each testfunction is executed
}


void {{ name }}::cleanup()
{
    // Called after every testfunction
}


{% for case in testCases %}

void {{ name }}::{{ case }}()
{


}

{% endfor %}
