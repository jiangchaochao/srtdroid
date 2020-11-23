package com.github.thibaultbee.srtdroid.models

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import com.github.thibaultbee.srtdroid.Srt
import com.github.thibaultbee.srtdroid.enums.*
import com.github.thibaultbee.srtdroid.models.rejectreason.InternalRejectReason
import com.github.thibaultbee.srtdroid.models.rejectreason.PredefinedRejectReason
import com.github.thibaultbee.srtdroid.models.rejectreason.UserDefinedRejectReason
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.File
import java.io.FileWriter
import java.io.IOException
import java.net.InetAddress
import java.net.SocketException
import java.net.StandardProtocolFamily
import java.util.concurrent.Callable
import java.util.concurrent.Executors
import java.util.concurrent.Future
import kotlin.random.Random


/*
 * Theses tests are written to check if SRT API can be called from the Kotlin part.
 */

@RunWith(AndroidJUnit4::class)
class SocketTest {
    private val srt = Srt()
    private lateinit var socket: Socket

    @Before
    fun setUp() {
        assert(srt.startUp() >= 0)
        socket = Socket()
        assertTrue(socket.isValid)
    }

    @After
    fun tearDown() {
        socket.close()
        assertEquals(srt.cleanUp(), 0)
    }

    @Test
    fun inetConstructorTest() {
        assertTrue(Socket(StandardProtocolFamily.INET).isValid)
    }

    @Test
    fun inet6ConstructorTest() {
        assertTrue(Socket(StandardProtocolFamily.INET6).isValid)
    }

    @Test
    fun bindTest() {
        socket.setSockFlag(SockOpt.TRANSTYPE, Transtype.FILE)
        socket.bind("127.0.3.1", 1234)
        assertTrue(socket.isBound)
    }

    @Test
    fun sockStatusTest() {
        assertEquals(SockStatus.INIT, socket.sockState)
        assertFalse(socket.isBound)
        socket.bind("127.0.3.1", 1235)
        assertEquals(SockStatus.OPENED, socket.sockState)
        assertTrue(socket.isBound)
    }

    @Test
    fun closeTest() {
        socket.close()
        assertTrue(socket.isClose)
    }

    @Test
    fun listenTest() {
        try {
            socket.listen(3)
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.EUNBOUNDSOCK.toString())
        }
        socket.bind("127.0.3.1", 1236)
        socket.listen(3)
    }

    @Test
    fun acceptTest() {
        try {
            socket.accept()
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOLISTEN.toString())
        }
    }

    @Test
    fun connectTest() {
        try {
            socket.connect("127.0.3.1", 1237)
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOSERVER.toString())
        }
        assertEquals(InternalRejectReason(RejectReasonCode.TIMEOUT), socket.rejectReason)
    }

    @Test
    fun rendezVousTest() {
        try {
            socket.rendezVous("0.0.0.0", "127.0.3.1", 1238)
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOSERVER.toString())
        }
    }

    @Test
    fun getPeerNameTest() {
        assertNull(socket.peerName)
    }

    @Test
    fun getSockNameTest() {
        assertNull(socket.sockName)
        socket.bind("127.0.3.1", 1239)
        assertEquals("127.0.3.1", socket.sockName!!.address.hostAddress)
        assertEquals(1239, socket.sockName!!.port)
    }

    @Test
    fun getSockOptTest() {
        try {
            socket.getSockFlag(SockOpt.TRANSTYPE)  // Write only property
            fail()
        } catch (e: IOException) {
        }
        assertEquals(true, socket.getSockFlag(SockOpt.RCVSYN))
        assertEquals(-1, socket.getSockFlag(SockOpt.SNDTIMEO))
        assertEquals(-1L, socket.getSockFlag(SockOpt.MAXBW))
        assertEquals(KMState.KM_S_UNSECURED, socket.getSockFlag(SockOpt.RCVKMSTATE))
        assertEquals("", socket.getSockFlag(SockOpt.STREAMID))
    }

    @Test
    fun setSockOptTest() {
        socket.setSockFlag(SockOpt.TRANSTYPE, Transtype.FILE)
        socket.setSockFlag(SockOpt.RCVSYN, true)
        socket.setSockFlag(SockOpt.SNDTIMEO, 100)
        socket.setSockFlag(SockOpt.MAXBW, 100L)
        socket.setSockFlag(SockOpt.STREAMID, "Hello")
    }

    @Test
    fun sendMsg1Test() {
        try {
            socket.send("Hello World !")
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOCONN.toString())
        }
    }

    @Test
    fun sendMsg2Test() {
        try {
            socket.send("Hello World !", -1, false)
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOCONN.toString())
        }
    }

    @Test
    fun sendMsg3Test() {
        val msgCtrl = MsgCtrl(boundary = 1, pktSeq = 1, no = 1)
        try {
            socket.send("Hello World !", msgCtrl)
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOCONN.toString())
        }
    }

    @Test
    fun recvTest() {
        try {
            socket.recv(
                4 /*Int nb bytes*/
            )
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOCONN.toString())
        }
    }

    @Test
    fun recvMsg2Test() {
        try {
            socket.recv(
                4 /*Int nb bytes*/,
                MsgCtrl(flags = 0, boundary = 0, pktSeq = 0, no = 10)
            )
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOCONN.toString())
        }
    }

    private fun createTestFile(): File {
        val myFile = File(
            InstrumentationRegistry.getInstrumentation().context.externalCacheDir,
            "FileToSend"
        )
        val fw = FileWriter(myFile)
        fw.write("Hello ! Did someone receive this message?")
        fw.close()
        return myFile
    }

    @Test
    fun sendFileTest() {
        try {
            socket.sendFile(createTestFile())
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOCONN.toString())
        }
    }

    @Test
    fun recvFileTest() {
        val myFile = File(
            InstrumentationRegistry.getInstrumentation().context.externalCacheDir,
            "FileToRecv"
        )
        try {
            socket.recvFile(myFile, 0, 1024)
            fail()
        } catch (e: SocketException) {
            assertEquals(e.message, ErrorType.ENOCONN.toString())
        }
    }

    @Test
    fun getRejectReasonTest() {
        assertEquals(InternalRejectReason(RejectReasonCode.UNKNOWN), socket.rejectReason)
    }

    @Test
    fun setRejectReasonTest() {
        socket.rejectReason = InternalRejectReason(RejectReasonCode.BADSECRET) // Generate an error
        socket.rejectReason = UserDefinedRejectReason(2)
        socket.rejectReason = PredefinedRejectReason(1)
    }

    @Test
    fun bstatsTest() {
        socket.bstats(true)
    }

    @Test
    fun bistatsTest() {
        socket.bistats(clear = true, instantaneous = false)
    }

    @Test
    fun connectionTimeTest() {
        assertEquals(0, socket.connectionTime)
    }

    @Test
    fun receiveBufferSizeTest() {
        assertNotEquals(0, socket.receiveBufferSize)
        socket.receiveBufferSize = 101568
        assertEquals(101568, socket.receiveBufferSize)
    }

    @Test
    fun reuseAddrTest() {
        socket.reuseAddress = true
        assertTrue(socket.reuseAddress)
        socket.reuseAddress = false
        assertFalse(socket.reuseAddress)
    }

    @Test
    fun sendBufferSizeTest() {
        assertNotEquals(0, socket.sendBufferSize)
        socket.sendBufferSize = 101568
        assertEquals(101568, socket.sendBufferSize)
    }

    @Test
    fun inputStreamTest() {
        val inputStream = socket.getInputStream()
        assertEquals(0, inputStream.read(ByteArray(0)))
        try {
            inputStream.read()
            fail()
        } catch (e: SocketException) {
        }
        try {
            inputStream.read(ByteArray(10))
            fail()
        } catch (e: SocketException) {
        }
        val server = MockServer()
        server.enqueue()
        socket.connect(InetAddress.getLoopbackAddress(), server.port)
        socket.close()
        assertEquals(0, inputStream.read(ByteArray(0)))
        try {
            inputStream.read()
            fail()
        } catch (e: SocketException) {
        }
        try {
            inputStream.read(ByteArray(10))
            fail()
        } catch (e: SocketException) {
        }
    }

    @Test
    fun outputStreamTest() {
        val outputStream = socket.getOutputStream()
        outputStream.write(ByteArray(0))
        try {
            outputStream.write(255)
            outputStream.write(ByteArray(10))
            fail()
        } catch (expected: SocketException) {
        }
    }

    @Test
    fun inputStreamLiveTest() {
        val server = InOutMockServer(Transtype.LIVE)
        val socket = Socket()
        val arraySize = socket.getSockFlag(SockOpt.PAYLOADSIZE) as Int
        val serverByteArray = ByteArray(arraySize)
        Random.Default.nextBytes(serverByteArray)
        server.enqueue(serverByteArray, 0)
        socket.setSockFlag(SockOpt.TRANSTYPE, Transtype.LIVE)
        socket.setSockFlag(SockOpt.RCVTIMEO, 1000)
        socket.connect(InetAddress.getLoopbackAddress(), server.port)
        val inputStream = socket.getInputStream()
        val byteArray = ByteArray(arraySize)
        assertEquals(arraySize, inputStream.read(byteArray))
        assertArrayEquals(serverByteArray, byteArray)
        try {
            inputStream.read()
            fail()
        } catch (e: SocketException) {
        }
        socket.close()
        inputStream.close()
        server.shutdown()
    }

    @Test
    fun outputStreamLiveTest() {
        val server = InOutMockServer(Transtype.LIVE)
        val socket = Socket()
        val arraySize = socket.getSockFlag(SockOpt.PAYLOADSIZE) as Int
        server.enqueue(ByteArray(arraySize), arraySize)
        socket.setSockFlag(SockOpt.TRANSTYPE, Transtype.LIVE)
        socket.connect(InetAddress.getLoopbackAddress(), server.port)
        val outputStream = socket.getOutputStream()
        outputStream.write(ByteArray(arraySize))
        socket.close()
        outputStream.close()
        try {
            outputStream.write(ByteArray(arraySize))
            fail()
        } catch (expected: IOException) {
        }
        server.shutdown()
    }

    @Test
    fun inputStreamFileTest() {
        val server = InOutMockServer(Transtype.FILE)
        server.enqueue(byteArrayOf(5, 3), 0)
        val socket = Socket()
        socket.setSockFlag(SockOpt.TRANSTYPE, Transtype.FILE)
        socket.setSockFlag(SockOpt.RCVTIMEO, 1000)
        socket.connect(InetAddress.getLoopbackAddress(), server.port)
        val inputStream = socket.getInputStream()
        assertEquals(5, inputStream.read())
        assertEquals(3, inputStream.read())
        try {
            inputStream.read()
            fail()
        } catch (e: SocketException) {
        }
        try {
            inputStream.read()
            fail()
        } catch (e: SocketException) {
        }
        socket.close()
        inputStream.close()

        try {
            inputStream.read()
            fail()
        } catch (e: SocketException) {
        }
        try {
            inputStream.read()
            fail()
        } catch (e: SocketException) {
        }
        server.shutdown()
    }

    @Test
    fun outputStreamFileTest() {
        val server = InOutMockServer(Transtype.FILE)
        server.enqueue(ByteArray(0), 3)
        val socket = Socket()
        socket.setSockFlag(SockOpt.TRANSTYPE, Transtype.FILE)
        socket.connect(InetAddress.getLoopbackAddress(), server.port)
        val outputStream = socket.getOutputStream()
        outputStream.write(5)
        outputStream.write(3)
        socket.close()
        outputStream.close()
        try {
            outputStream.write(9)
            fail()
        } catch (expected: IOException) {
        }
        server.shutdown()
    }

    internal class MockServer {
        private val executor = Executors.newCachedThreadPool()
        private val serverSocket = Socket()
        val port: Int
        private var socket: Socket? = null

        init {
            serverSocket.reuseAddress = true
            serverSocket.bind(InetAddress.getLoopbackAddress(), 0)
            port = serverSocket.localPort
        }

        fun enqueue(): Future<Unit> {
            return executor.submit(Callable {
                serverSocket.listen(1)
                val pair = serverSocket.accept()
                assertNotNull(pair.second)
                socket = pair.first
            })
        }

        fun shutdown() {
            socket?.close()
            serverSocket.close()
            executor.shutdown()
        }
    }

    internal class InOutMockServer(transtype: Transtype) {
        private val executor = Executors.newCachedThreadPool()
        private val serverSocket = Socket()
        private var socket: Socket? = null
        val port: Int

        init {
            serverSocket.reuseAddress = true
            serverSocket.setSockFlag(SockOpt.TRANSTYPE, transtype)
            serverSocket.bind(InetAddress.getLoopbackAddress(), 0)
            port = serverSocket.localPort
        }

        fun enqueue(sendBytes: ByteArray, receiveByteCount: Int): Future<ByteArray?> {
            return executor.submit(Callable<ByteArray?> {
                serverSocket.listen(1)
                val pair = serverSocket.accept()
                assertNotNull(pair.second)
                socket = pair.first
                val outputStream = socket?.getOutputStream()
                if (outputStream != null) {
                    assertEquals(sendBytes.size, outputStream.write(sendBytes))
                    val inputStream = socket?.getInputStream()
                    if (inputStream != null) {
                        val result = ByteArray(receiveByteCount)
                        var total = 0
                        while (total < receiveByteCount) {
                            total += inputStream.read(result, total, result.size - total)
                        }
                        result
                    }
                }
                ByteArray(0)

            })
        }

        fun shutdown() {
            socket?.close()
            serverSocket.close()
            executor.shutdown()
        }
    }

}