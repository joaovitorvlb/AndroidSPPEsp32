package com.example.appbluetooth;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    private static final  int SOLICITA_ATIVACAO = 1;
    private static final  int SOLICITA_CONEXAO = 2;
    private static final  int MESSAGE_READ = 3;
    BluetoothAdapter meuBluetooth = null;
    BluetoothDevice meuDevice = null;
    BluetoothSocket meuSocket = null;
    boolean conexao = false;
    private static String mac = null;
    UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");
    Button btnConexao;
    Button btnEnviar;
    public TextView txtMsg;

    ConnectedThread connectedThread;
    Handler mHandler;
    StringBuilder dadosBluetooth = new StringBuilder();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnConexao = findViewById(R.id.btnCon);
        btnEnviar = findViewById(R.id.btnSend);
        txtMsg = findViewById(R.id.txtMsg);

        meuBluetooth = BluetoothAdapter.getDefaultAdapter();
        if (meuBluetooth == null){
            Toast.makeText(getApplicationContext(),"Não existe Bluetooth", Toast.LENGTH_LONG).show();
        }else if (!meuBluetooth.isEnabled()){
            Intent atibaBluetooth = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(atibaBluetooth, SOLICITA_ATIVACAO);
        }

        btnConexao.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if(conexao){
                    // dewconectar
                    try {
                        meuSocket.close();
                        Toast.makeText(getApplicationContext(),"Desconectado !" , Toast.LENGTH_LONG).show();
                        conexao = false;
                        btnConexao.setText("Conectar");
                    }catch (IOException erro){
                        Toast.makeText(getApplicationContext(),"ocorreu um erro:" + erro, Toast.LENGTH_LONG).show();
                        conexao = true;
                        btnConexao.setText("Desconectar");
                    }
                }else{
                    // Conectar
                    Intent abreLista = new Intent(MainActivity.this, ListaDispositivos.class);
                    startActivityForResult(abreLista,SOLICITA_CONEXAO);
                    conexao = true;
                    btnConexao.setText("Desconectar");
                }
            }
        });

        btnEnviar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String send = "LED1";
                if (conexao){
                    connectedThread.write(send);
                }else{
                    Toast.makeText(getApplicationContext(),"O bluetooth não está conectado !" , Toast.LENGTH_LONG).show();
                }
            }
        });

        mHandler = new Handler(){
            @Override
            public void handleMessage(@NonNull Message msg) {
                //super.handleMessage(msg);
                if (msg.what == MESSAGE_READ){
                    String recebidos = (String)msg.obj;
                    dadosBluetooth.append(recebidos);
                    Log.d("String",recebidos);
                    int fim = dadosBluetooth.indexOf("}");
                    if (fim >0){
                        String dadosCompletos = dadosBluetooth.substring(0, fim);
                        int tam = dadosCompletos.length();
                        if(dadosBluetooth.charAt(0) == '{'){
                            String dadosFinais = dadosBluetooth.substring(1, tam);
                            Log.d("Recebidos",dadosFinais);
                            txtMsg.setText(dadosFinais);
                        }
                        dadosBluetooth = new StringBuilder();
                    }
                }
            }
        };
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        switch(requestCode){
            case SOLICITA_ATIVACAO:
                if(resultCode == Activity.RESULT_OK){
                    Toast.makeText(getApplicationContext(),"O Bluetooth foi Ativado", Toast.LENGTH_LONG).show();
                }else{
                    Toast.makeText(getApplicationContext(),"O Bluetooth não foi Ativado", Toast.LENGTH_LONG).show();
                    finish();
                }
            break;

            case SOLICITA_CONEXAO:
                if(resultCode == Activity.RESULT_OK){
                    mac = data.getExtras().getString(ListaDispositivos.ENDERECO_MAC);
                    // Toast.makeText(getApplicationContext(),"MAC final:" + mac, Toast.LENGTH_LONG).show();
                    meuDevice = meuBluetooth.getRemoteDevice(mac);

                    try {
                        meuSocket = meuDevice.createRfcommSocketToServiceRecord(uuid);
                        meuSocket.connect();
                        conexao = true;
                        Toast.makeText(getApplicationContext(),"Conectado com:" + mac, Toast.LENGTH_LONG).show();
                        connectedThread = new ConnectedThread(meuSocket);
                        connectedThread.start();
                    }catch(IOException erro){
                        conexao = false;
                        Toast.makeText(getApplicationContext(),"ocorreu um erro" + erro, Toast.LENGTH_LONG).show();
                    }
                }else{
                    Toast.makeText(getApplicationContext(),"falha ao obter o MAC", Toast.LENGTH_LONG).show();
                    conexao = false;
                }
        }
    }

    private class ConnectedThread extends Thread {

        private final InputStream mmInStream;
        private final OutputStream mmOutStream;
        private byte[] mmBuffer; // mmBuffer store for the stream

        public ConnectedThread(BluetoothSocket socket) {
            meuSocket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            try {
                tmpIn = socket.getInputStream();
            } catch (IOException e) {
                // Log.e(TAG, "Error occurred when creating input stream", e);
            }
            try {
                tmpOut = socket.getOutputStream();
            } catch (IOException e) {
                // Log.e(TAG, "Error occurred when creating output stream", e);
            }

            mmInStream = tmpIn;
            mmOutStream = tmpOut;
        }

        public void run() {
            mmBuffer = new byte[1024];
            int numBytes; // bytes returned from read()

            while (true) {
                try {
                    // Read from the InputStream.
                    numBytes = mmInStream.read(mmBuffer);
                    String dados = new String(mmBuffer, 0 ,numBytes);
                    //Log.d("dados",dados);
                    mHandler.obtainMessage(MESSAGE_READ, numBytes, -1, dados).sendToTarget();
                } catch (IOException e) {
                    //Log.d(TAG, "Input stream was disconnected", e);
                    break;
                }
            }
        }

        // Call this from the main activity to send data to the remote device.
        // public void write(byte[] bytes) {
        public void write(String enviar) {
            byte[] msgBuffer = enviar.getBytes();
            try {
                mmOutStream.write(msgBuffer);
            } catch (IOException e) {

            }
        }

        // Call this method from the main activity to shut down the connection.
        public void cancel() {
            try {
                meuSocket.close();
            } catch (IOException e) {
                //Log.e(TAG, "Could not close the connect socket", e);
            }
        }
    }
}