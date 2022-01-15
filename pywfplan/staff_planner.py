from .pywfplan_ext import ShiftRule, PlanExt, TargetExt, StaffPlannerExt


class StaffPlanner:
    """
    Staff planner
    """

    def __init__(self):
        self.offset_ = 0
        self.agents_ = {}
        self.target_ = None
        self.result_ = None
        self.report_ = None


    def addAgentRule(self, code : str, rule : ShiftRule):
        """
        Specify a shift assignment rule for the agent
        """
        rule_offset = max([s.t1() for s in rule.shifts()]) - 24*60

        if rule_offset > self.offset_:
            self.offset_ = rule_offset

        self.agents_[code] = rule


    def setStaffingTarget(self, target, days : int = 7, slot_length : int = 15):
        """
        Set target staffing
        """
        self.target_ = TargetExt(slot_length, days, target)


    def run(self, annealing_schedule : float = 0.9, comfort_energy_weight : float =0.2):
        """
        Run optimization
        """
        plan = PlanExt(self.offset_, self.agents_.keys(), self.target_)
        staff_planner = StaffPlannerExt("", plan, annealing_schedule, comfort_energy_weight)

        for code, rule in self.agents_.items():
            staff_planner.setAgentSampler(code, rule)

        staff_planner.run()

        self.result_ = staff_planner.getPlan()
        self.report_ = staff_planner.getReport()


    def getAgentPlan(self, agent_code : str) -> list[str]:
        """
        Get the optimized plan for agent
        """
        if self.result_ is None:
            raise Exception("the plan has not been optimized yet")

        return [s.code() for s in self.result_.getAgentPlan(agent_code)]


    def getTargetStaffing(self) -> list[float]:
        """
        Get the optimized staffing curve
        """
        if self.result_ is None:
            raise Exception("the plan has not been optimized yet")

        return self.result_.getTargetStaffing()


    def getPlannedStaffing(self) -> list[float]:
        """
        Get the optimized staffing curve
        """
        if self.result_ is None:
            raise Exception("the plan has not been optimized yet")

        return self.result_.getPlannedStaffing()


    def getReport(self) -> str:
        """
        Get optimization report
        """
        if self.result_ is None:
            raise Exception("the plan has not been optimized yet")

        return self.report_
