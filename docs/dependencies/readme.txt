This is a dependency graph for lwiso. The upper cell in each record contains the component name, and the lower cell contains the error code space for that component. A solid line indicates a link dependency, and a dashed line indicates a dlopen dependency.

dependencies.png was rendered using graphviz from dependencies.dot. If you wish to make changes, modify the .dot file (the syntax is described at www.graphviz.org/Documentation.php), then regenerate the .png by running:
dot dependencies.dot -Tpng -o dependencies.png
