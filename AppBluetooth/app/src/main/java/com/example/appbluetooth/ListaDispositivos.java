package com.example.appbluetooth;

import android.app.ListActivity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;

import java.util.Set;

public class ListaDispositivos extends ListActivity {

    private BluetoothAdapter bluetoothAdapter = null;
        static String ENDERECO_MAC = null;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ArrayAdapter<String> arrayBluetooth = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1);
        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        Set<BluetoothDevice> dispositivosPareados = bluetoothAdapter.getBondedDevices();

        if(dispositivosPareados.size() > 0){
            for(BluetoothDevice dispositivo : dispositivosPareados){
                String nomeBt = dispositivo.getName();
                String macBt = dispositivo.getAddress();
                arrayBluetooth.add(nomeBt +"\n"+ macBt);
            }
        }
        setListAdapter(arrayBluetooth);
    }

    @Override
    protected void onListItemClick(ListView l, View v, int position, long id) {
        super.onListItemClick(l, v, position, id);

        String informacao = ((TextView) v).getText().toString();
        // Toast.makeText(getApplicationContext(),"Info:" + informacao, Toast.LENGTH_LONG).show();
        String macAddress = informacao.substring(informacao.length() -17);
        // Toast.makeText(getApplicationContext(),"mac:" + macAddress, Toast.LENGTH_LONG).show();
        Intent retornamac = new Intent();
        retornamac.putExtra(ENDERECO_MAC, macAddress);
        setResult(RESULT_OK, retornamac);
        finish();
    }
}
