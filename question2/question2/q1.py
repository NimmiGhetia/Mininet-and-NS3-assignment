
from mininet.net import Mininet
from mininet.node import Controller, RemoteController, OVSController
from mininet.node import CPULimitedHost, Host, Node
from mininet.node import OVSKernelSwitch, UserSwitch
from mininet.node import IVSSwitch
from mininet.cli import CLI
from mininet.log import setLogLevel, info
from mininet.link import TCLink, Intf
from subprocess import call

import sys

def myNetwork(n,m):

    net = Mininet( topo=None,
                   link=TCLink,
                   build=False,
                   ipBase='10.0.0.0/8'
                   )  

    """link=TCLink, must be added in order to change link  parameters eg. bw,delay etc.""" 
    
    info( '*** Adding controller\n' )
    c0=net.addController(name='c0',
                      controller=RemoteController,
                      ip='127.0.0.1',
                      protocol='tcp',
                      port=6633)

    info( '*** Add switches\n')
    s1 = net.addSwitch('s1', cls=OVSKernelSwitch)
    s2 = net.addSwitch('s2', cls=OVSKernelSwitch)
    info( '*** Add hosts\n')
    hosts = []
    for i in range(n+m):
        hostname = 'h'+str(i)
	hostip = '10.0.0.'+str(i+1)
        hst = net.addHost(hostname, cls=Host, ip=hostip, defaultRoute=None)
	hosts.append(hst)

    info( '*** Add links\n')
    for i in range(n):
	net.addLink(hosts[i],s1)
	print('added host '+str(i)+' to switch s1')
    for i in range(n,n+m):
	net.addLink(hosts[i],s2)
	print('added host '+str(i)+' to switch s2')
    net.addLink(s1,s2)
    info( '*** Starting network\n')
    net.build()
    info( '*** Starting controllers\n')
    for controller in net.controllers:
        controller.start()

    info( '*** Starting switches\n')
    net.get('s1').start([c0])
    net.get('s2').start([c0])
    info( '*** Post configure switches and hosts\n')

    CLI(net)
    net.stop()
def main():
    setLogLevel('info')
    args = sys.argv[1:] 
    n = int(args[0]) 
    m = int(args[1])
    myNetwork(n,m)	

if __name__ == '__main__':
    main()
