import re
from typing import Dict
from datetime import time
from .pywfplan_ext import ShiftExt, ShiftRule


def parse_shift_spec(spans : str = ""):
    """
    Parse a shift specification line of the form

    '09:00-12:00, 15:00-18:00, ...'
    """
    if spans.replace(" ", "") == "":
        return []

    p = re.compile("^(\d\d):(\d\d)-(\d\d):(\d\d)$")
    def parse_interval(s):
        m = p.match(s.replace(" ", ""))
        if m is not None:
            t0 = int(m.group(1))*60 + int(m.group(2))
            t1 = int(m.group(3))*60 + int(m.group(4))
            return [t0, t1]
        else:
            raise Exception("invalid shift specification {}".format(s))

    return [parse_interval(s) for s in spans.split(",")]


class Shift(ShiftRule):
    """
    The shift class represents the work that can be assigned to an agent
    over a period of time. Shifts can be composed to form `rules`.

    Examples:

    A working shift for a single day over one or more time spans

    A1 = Shift.fromSpec("A1", "09:00-12:00")
    B1 = Shift.fromSpec("B1", "09:00-12:00, 13:30-17:30")

    A rest shift

    R = Shift.fromSpec("R")

    A shift assignment for a single day with 3 choices

    s = A1 + A2 + A3

    A shift assignment over a week with working shifts from mon to fri and rest
    over the weekend

    W * W * W * W * W * R * R
    """

    def __init__(self, shift : ShiftExt, attrs : Dict = {}):
        """
        Create a shift with a code and a list of spans and optionally some
        attributes
        """
        self.attrs = attrs
        super().__init__(shift)


    @classmethod
    def fromSpec(cls, code : str, spec : str = "", attrs : Dict = {}):
        """
        Create a shift with a code and a specification line like

        '09:00-12:00, 15:30-19:30'

        If no specification is given the shift is assumed to be a rest shift
        """
        return cls(ShiftExt(code, parse_shift_spec(spec)), attrs)


    def is_work(self):
        """
        Check if it is a work shift
        """
        if not self.is_literal():
            raise Exception("cannot chek if work/rest flag of composed rule")

        return self.shift().work()


    def start_time(self) -> time:
        """
        Get shift start time
        """
        if not self.is_work():
            raise Exception("not a work shift")

        if not self.is_literal():
            raise Exception("cannot get single day start time from composed rule")

        t0 = self.shift().t0()

        return time(hour=t0 // 60, minute=t0 % 60)


    def end_time(self) -> time:
        """
        Get shift end time
        """
        if not self.is_work():
            raise Exception("not a work shift")

        if not self.is_literal():
            raise Exception("cannot get single day end time from composed rule")

        t1 = self.shift().t1()

        return time(hour=t1 // 60, minute=t1 % 60)
