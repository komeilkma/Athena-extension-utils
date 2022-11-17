"""
Formatting Console Output
=========================

A collection of utilities for formatting console output with colours.

This module is mostly used in the inner workings of **atlasplots** utility
functions but you can use it in your plotting scripts too!


**Supported Colours**

+------------+----------------------+
| Colour     | Command              |
+============+======================+
| Red        | ``bcolor.red``       |
+------------+----------------------+
| Green      | ``bcolor.green``     |
+------------+----------------------+
| Yellow     | ``bcolor.yellow``    |
+------------+----------------------+
| Blue       | ``bcolor.blue``      |
+------------+----------------------+
| Magenta    | ``bcolor.magenta``   |
+------------+----------------------+
| Cyan       | ``bcolor.cyan``      |
+------------+----------------------+
| White      | ``bcolor.white``     |
+------------+----------------------+


**Colour aliases**

+----------------------+----------+
| Command              | Colour   |
+======================+==========+
| ``bcolor.HEADER``    | Blue     |
+----------------------+----------+
| ``bcolor.OK``        | Green    |
+----------------------+----------+
| ``bcolor.WARNING``   | Yellow   |
+----------------------+----------+
| ``bcolor.ERROR``     | Red      |
+----------------------+----------+
| ``bcolor.FATAL``     | Red      |
+----------------------+----------+


**Pre-formatted Strings**

+----------------------+--------------------------+
| Command              | Returns                  |
+======================+==========================+
| ``bcolor.ok()``      |  ``OK`` (in green)       |
+----------------------+--------------------------+
| ``bcolor.warning()`` | ``Warning`` (in yellow)  |
+----------------------+--------------------------+
| ``bcolor.error()``   | ``Error`` (in red)       |
+----------------------+--------------------------+



**Other Text Formatting**

+------------+----------------------+
| Style      | Command              |
+============+======================+
| Bold       | ``bcolor.bold``      |
+------------+----------------------+
| Faint Text | ``bcolor.faint``     |
+------------+----------------------+
| Italic     | ``bcolor.italic``    |
+------------+----------------------+
| Underline  | ``bcolor.underline`` |
+------------+----------------------+


Note
----

Always end the string you want formatted with ``bcolor.end``.


Examples
--------

Import the ``bcolor`` class:

>>> from atlasplots.console import bcolor

Print text with colour:

>>> print(bcolor.blue + "Blue" + bcolor.end)
>>> print(bcolor.red + "Red " + bcolor.green + "Green" + bcolor.end)

You can also use colour aliases for printing OK/Warning/Error messages:

>>> print(bcolor.OK + "OK" + bcolor.end)
>>> print(bcolor.WARNING + "Warning" + bcolor.end)
>>> print(bcolor.ERROR + "Error" + bcolor.end)

Or similarly using Python's string formatting:

>>> print("{}  Something went wrong!".format(bcolor.ERROR + "Error" + bcolor.end))

Or you can use the pre-formatted strings as a shortcut:

>>> print("{}  Something went wrong!".format(bcolor.error()))

"""


class bcolor:
    # Colours
    red = "\033[91m"
    green = "\033[92m"
    yellow = "\033[93m"
    blue = "\033[94m"
    magenta = "\033[95m"
    cyan = "\033[96m"
    white = "\033[97m"

    # Colour aliases
    HEADER = blue
    OK = green
    WARNING = yellow
    ERROR = red
    FATAL = red

    # Text formatting
    bold = "\033[1m"
    faint = "\033[2m"
    italic = "\033[3m"
    underline = "\033[4m"

    # End formatting
    end = "\033[0m"

    # Pre-formatted strings
    @staticmethod
    def ok():
        return bcolor.OK + "OK" + bcolor.end

    @staticmethod
    def warning():
        return bcolor.WARNING + "Warning" + bcolor.end

    @staticmethod
    def error():
        return bcolor.ERROR + "Error" + bcolor.end
