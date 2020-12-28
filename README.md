<h1>TVTorrent</h1>
<p>A simple bittorrent client oriented towards shows and episode-based downloading. Work in progress.</p>
<h3>Dependencies</h3>
<ul>
    <li> gtkmm3 </li>
    <li> libtorrent </li>
    <li> libtorrent-rasterbar </li>
    <li> jsoncpp </li>
    <li> boost </li>
</ul>
<code>These were the dependencies on Arch linux, please adjust according to your OS.</code>
<h3>Build & Install instructions</h3>
<p>Make sure all the dependencies are installed!</p>
<p>Clone the source from GitHub:</p>
<code>$ git clone https://github.com/sheepkill15/tvtorrent.git tvtorrent</code>
<br/>
<p>Cd into cloned directory:</p>
<code>$ cd tvtorrent</code>
<br/>
<p>Create a build directory and cd into it:</p>
<code>$ mkdir build</code>
<code>$ cd build</code>
<br/>
<p>Run cmake on root directory:</p>
<p>For debug version:</p>
<code>$ cmake -DCMAKE_BUILD_TYPE=Debug ../</code>
<br/>
<p>For release version:</p>
<code>$ cmake -DCMAKE_BUILD_TYPE=Release ../</code>
<br/>
<p>Now run make in current directory:</p>
<code>$ make</code>
<br/>
<p>And now install the required files</p>
<code>$ cmake --install .</code>
<br/>
<h3>Usage</h3>
<p>To run from terminal, use:</p>
<code>$ tvtorrent &lt;filename/magnet-url/nothing&gt;</code>