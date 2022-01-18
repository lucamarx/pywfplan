Pywfplan: Call center planning
==============================

Pywfplan is a Python library that helps planning the activity of a call center.

It is based on a C++ application I wrote a few years ago that has been used in
production to plan call centers of sizes between 50 to 200 agents.

Pywfplan is a stripped down version of that application that retains the basic
functionality and exports it as a Python module.

Installation
------------

Ensure that you have a C++ compiler and the boost library installed, then run

    python setup.py install

Or use the provided docker file:

    docker build -t pywfplan .
    docker run -p 8888:8888 -it pywfplan

Quick Start
-----------

Suppose you are given the following

- a list of *agents*
- a list of *shift definitions*
- a *staffing curve* that tells how many agents must be at work at a given time
- some more requirements (for example about specific contract types or about how
  rests are assigned)

and you are asked to produce an *optimal plan* for one week that obeys the
requirements. That is you have to assign a shift to every agent for each day.


The plan should be optimal in the sense that at any given time there should be
just enough agents at work to ensure that calls are being answered but no more
than them and of course at no time there should be less than the minimum number
of agents required.


In pywfplan the `Shift` class provides a simple notation to encode shift and
requirements:

```python
# 4 hours shifts
A1 = Shift.fromSpec("A1", "09:00-12:00")
A2 = Shift.fromSpec("A2", "09:30-12:30")
A3 = Shift.fromSpec("A3", "10:00-14:00")
...
# 8 hours shifts with a pause for lunch
M1 = Shift.fromSpec("M1", "09:00-12:00,13:00-17:00")
M2 = Shift.fromSpec("M2", "09:30-12:30,13:30-17:30")
M3 = Shift.fromSpec("M3", "10:00-13:00,14:00-18:00")
...
# a rest shift
R =  Shift.fromSpec("R")
```

then shifts can be *composed* with `+` and `*` to define complex assignments:

```python
# assign either A1 or A2 or A3 on a single day
W = A1 + A2 + A3

# assign A1 on day 1, A2 on days 2-5 and R on day 7
W = A1 * A2 * A2 * A2 * A2 * A2 * R

# assign a 4 days shift on days 1-5, a 8 hour shift on day 6 and rest on day 7
A = A1 + A2 + A3
M = M1 + M2 + M3
W = A * A * A * A * A * M * R
```

The `StaffPlanner` class is used to produce the optimal plan:

```python
planner = StaffPlanner()
```

you set the target staffing curve

```python
planner.setStaffingTarget(target, days=7)
```

for each agent you specify its assignment rule

```python
# this agent works 4 hours from mon to fri
W = A1 + A2 + A3
rule = W * W * W * W * W * R * R

planner.addAgentRule(agent_code, rule)
```

the optimization is run with

```python
planner.run()
```

and the optimal plan for each agent is retrieved with

```python
agent_plan = planner.getAgentPlan(agent_code)
```


Internally pywfplan converts the assignment rule into a *finite state machine*
that can be used to sample all the valid plans for that agent.


The planner then uses *simulated annealing* to minimize the average squared
difference between the sampled plan staffing and the target staffing curves.
