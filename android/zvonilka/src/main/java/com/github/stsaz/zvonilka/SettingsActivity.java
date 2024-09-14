/** zvonilka/Android
2024, Simon Zolin */

package com.github.stsaz.zvonilka;

import android.os.Bundle;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import com.github.stsaz.zvonilka.databinding.SettingsBinding;

public class SettingsActivity extends AppCompatActivity {
	private static final String TAG = "zvonilka.SettingsActivity";
	private Core core;
	private SettingsBinding b;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		b = SettingsBinding.inflate(getLayoutInflater());
		setContentView(b.getRoot());

		ActionBar actionBar = getSupportActionBar();
		if (actionBar != null)
			actionBar.setDisplayHomeAsUpEnabled(true);

		core = Core.ref();
		load();
	}

	@Override
	protected void onPause() {
		save();
		super.onPause();
	}

	@Override
	protected void onDestroy() {
		core.unref();
		super.onDestroy();
	}

	private void load() {
		// Connection
		b.ePort.setText(Util.int_to_str(core.settings.tcp_port));

		// Audio
		b.eBuffer.setText(Util.int_to_str(core.settings.a_buffer));
		b.eQuality.setText(Util.int_to_str(core.settings.a_quality));
		b.eGain.setText(Util.int_to_str(core.settings.a_gain));
	}

	private void save() {
		// Connection
		core.settings.tcp_port = Util.str_to_uint(b.ePort.getText().toString(), -1);

		// Audio
		core.settings.a_buffer = Util.str_to_uint(b.eBuffer.getText().toString(), -1);
		core.settings.a_quality = Util.str_to_uint(b.eQuality.getText().toString(), -1);
		core.settings.a_gain = Util.str_to_uint(b.eGain.getText().toString(), -1);

		core.settings.normalize();
	}
}
