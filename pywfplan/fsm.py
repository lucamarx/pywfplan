import graphviz
from .pywfplan_ext import Re, FsmExt

class Fsm(FsmExt):
    """
    Wrapper class over C++ implementation of a finite state machine
    """

    def __init__(self, regexp : Re):
        """
        Create a new finite state machine compiling the regexp with Brzozowski
        algorithm
        """
        super().__init__(regexp)


    def samples(self, n=10):
        """
        Take n samples
        """
        return ["".join(self.sample()) for _ in range(0,n)]


    def diagram(self):
        """
        Display the state transition diagram
        """
        return graphviz.Source(self.__repr__())
