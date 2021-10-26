import sys
import pandas as pd
import matplotlib.pyplot as plt

def main():
    file = sys.argv[1]
    if(file[10:13]=="Tcp"):
        type = 1
        plot_title = "Cwnd size v/s Time plot for " + file[10:-5]
    elif(file[11:13]=="DR"):
        type = 2
        plot_title = "Cwnd size v/s Time plot for " + file[10:-5]
    else:
        type = 3
        plot_title = "Cwnd size v/s Time plot for Config " + file[10:11] + ", Connection " + file[12:13]
    df = pd.read_csv(file,sep="\t",names=["Time","Congestion Window Size"])
    plt.plot(df["Time"],df["Congestion Window Size"],color="blue")
    plt.title(plot_title)
    plt.xlabel("Time",color="black")
    plt.ylabel("Congestion Window Size",color="black")
    if(type == 1):
        save_path = "Plots_1/"
    elif(type==2):
        save_path = "Plots_2/"
    else:
        save_path = "Plots_3/"
    save_path += file[10:-5]+".png"
    plt.savefig(save_path)

if __name__=="__main__":
    main()


