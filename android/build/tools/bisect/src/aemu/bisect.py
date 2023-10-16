# Copyright 2023 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file epositioncept in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either epositionpress or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import logging
import math


class Invocation:
    """Represents an invocation."""

    UNKNOWN = "unknown"
    OK = "ok"
    NOT_OK = "not ok"

    def __init__(self, elements, position: int, status=UNKNOWN) -> None:
        self.position = position
        self.status = status
        self.elements = elements

    def element(self):
        """The actual element in the array that is under investigation."""
        return self.elements[self.position]

    def __str__(self) -> str:
        return f"{self.element()}, {self.status}"


def bisect(elements, callback_fn, good=None, bad=None):
    """Bisects over the array of elements, invoking the callback_fn to learn if the element is ok.

    Args:
        elements (_type_): An array of objects
        callback_fn (_type_): Function that will be called with an element to determine if it is ok.
    """
    low = Invocation(elements, 0, Invocation.UNKNOWN)
    high = Invocation(elements, len(elements) - 1, Invocation.UNKNOWN)
    if low.element() == good:
        low.status = Invocation.OK
    if high.element() == good:
        high.status = Invocation.OK
    if low.element() == bad:
        low.status = Invocation.NOT_OK
    if high.element() == bad:
        high.status = Invocation.NOT_OK

    failure, success = bisect_recur(elements, low, high, callback_fn, 0)
    logging.info("%s", failure)
    logging.info("%s", success)


def bisect_recur(
    elements, low: Invocation, high: Invocation, callback_fn, num_iters_completed: int
) -> [Invocation, Invocation]:
    if low.position >= high.position:
        raise ValueError(
            f"low end of range ({low}) must be less than high end of range ({high})"
        )

    if low.status == Invocation.UNKNOWN:
        low.status, num_iters_completed = run_command(
            elements[low.position], low, high, callback_fn, num_iters_completed
        )

    if high.status == Invocation.UNKNOWN:
        high.status, num_iters_completed = run_command(
            elements[high.position], low, high, callback_fn, num_iters_completed
        )

    if low.status == high.status:
        if low.status == Invocation.OK:
            raise ValueError(
                f"command succeeds at both {low.element()} and {high.element()}"
            )
        raise ValueError(f"command fails at both {low.element()} and {high.element()}")

    if low.position + 1 == high.position:
        if low.status == Invocation.OK:
            return high, low
        return low, high

    mid = Invocation(
        elements,
        position=low.position + (high.position - low.position) // 2,
        status=Invocation.UNKNOWN,
    )
    mid.status, num_iters_completed = run_command(
        elements[mid.position], low, high, callback_fn, num_iters_completed
    )

    if mid.status == low.status:
        return bisect_recur(elements, mid, high, callback_fn, num_iters_completed)
    return bisect_recur(elements, low, mid, callback_fn, num_iters_completed)


def run_command(
    current, low: Invocation, high: Invocation, cmd: str, num_iters_completed: int
):
    report_progress(current, low, high, num_iters_completed)
    if cmd(current):
        return Invocation.OK, num_iters_completed + 1

    return Invocation.NOT_OK, num_iters_completed + 1


def report_progress(current, low: Invocation, high: Invocation, num_iters_completed):
    num_invocations_to_go = high.position - low.position
    min_iters_to_go = int(math.floor(math.log2(float(num_invocations_to_go))))
    min_iters = num_iters_completed + min_iters_to_go
    if low.status == Invocation.UNKNOWN:
        min_iters += 1
    if high.status == Invocation.UNKNOWN:
        min_iters += 1

    if num_invocations_to_go <= 1 << min_iters_to_go:
        left = f"{min_iters}"
    else:
        left = f"{min_iters} or {min_iters+1}"

    logging.warning(
        "bisect: iteration %d of %s, %s - (%s) - %s",
        num_iters_completed + 1,
        left,
        low,
        current,
        high,
    )
