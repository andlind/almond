from flask import Blueprint
import matplotlib.pyplot as plt
import base64
from io import BytesIO
from matplotlib.figure import Figure
from flask import render_template

show_graph = Blueprint('show_graph', __name__, template_folder='templates')

@show_graph.route('/howru/monitoring/graph', methods=['GET'])
def index():
    x = [1,2,3]
    # corresponding y axis values
    y = [2,4,1]

    # plotting the points
    plt.plot(x, y)

    # naming the x axis
    plt.xlabel('x - axis')
    # naming the y axis
    plt.ylabel('y - axis')

    # giving a title to my graph
    plt.title('My first graph!')

    # function to show the plot
    #return(plt.show())
    #fig = Figure()
    #ax = fig.subplots()
    #ax.plot([1, 2])
    # Save it to a temporary buffer.
    buf = BytesIO()
    #fig.savefig(buf, format="png")
    plt.savefig(buf, format="png")
    plt.savefig('static/images/new_plot.png')
    return render_template('graph.html', name='new_plot', url='static/images/new_plot.png')
    # Embed the result in the html output.
    #data = base64.b64encode(buf.getbuffer()).decode("ascii")
    #return f"<img src='data:image/png;base64,{data}'/>"
