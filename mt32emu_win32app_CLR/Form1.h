#pragma once


namespace mt32emu_win32app_CLR {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	static MT32Emu::MidiSynth midiSynth;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			cbDACInputMode->SelectedIndex = 0;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^  bClose;
	private: System::Windows::Forms::Button^  bReset;
	private: System::Windows::Forms::Button^  bApply;





	private: System::Windows::Forms::NumericUpDown^  nudSampleRate;
	private: System::Windows::Forms::NumericUpDown^  nudMidiInPort;
	private: System::Windows::Forms::NumericUpDown^  nudVolume;
	private: System::Windows::Forms::NumericUpDown^  nudBufferLatency;




	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;

	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::CheckBox^  chbReverbEnabled;

	private: System::Windows::Forms::ComboBox^  cbDACInputMode;
	private: System::Windows::Forms::CheckBox^  chbShowConsole;


	protected: 

	protected: 

	protected: 

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->bClose = (gcnew System::Windows::Forms::Button());
			this->bReset = (gcnew System::Windows::Forms::Button());
			this->bApply = (gcnew System::Windows::Forms::Button());
			this->nudSampleRate = (gcnew System::Windows::Forms::NumericUpDown());
			this->nudMidiInPort = (gcnew System::Windows::Forms::NumericUpDown());
			this->nudVolume = (gcnew System::Windows::Forms::NumericUpDown());
			this->nudBufferLatency = (gcnew System::Windows::Forms::NumericUpDown());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->chbReverbEnabled = (gcnew System::Windows::Forms::CheckBox());
			this->cbDACInputMode = (gcnew System::Windows::Forms::ComboBox());
			this->chbShowConsole = (gcnew System::Windows::Forms::CheckBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudSampleRate))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudMidiInPort))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudVolume))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudBufferLatency))->BeginInit();
			this->SuspendLayout();
			// 
			// bClose
			// 
			this->bClose->DialogResult = System::Windows::Forms::DialogResult::Cancel;
			this->bClose->Location = System::Drawing::Point(205, 185);
			this->bClose->Name = L"bClose";
			this->bClose->Size = System::Drawing::Size(75, 23);
			this->bClose->TabIndex = 8;
			this->bClose->Text = L"Close";
			this->bClose->UseVisualStyleBackColor = true;
			this->bClose->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// bReset
			// 
			this->bReset->DialogResult = System::Windows::Forms::DialogResult::OK;
			this->bReset->Location = System::Drawing::Point(108, 185);
			this->bReset->Name = L"bReset";
			this->bReset->Size = System::Drawing::Size(75, 23);
			this->bReset->TabIndex = 7;
			this->bReset->Text = L"Reset Synth";
			this->bReset->UseVisualStyleBackColor = true;
			this->bReset->Click += gcnew System::EventHandler(this, &Form1::button1_Click_1);
			// 
			// bApply
			// 
			this->bApply->Location = System::Drawing::Point(12, 185);
			this->bApply->Name = L"bApply";
			this->bApply->Size = System::Drawing::Size(75, 23);
			this->bApply->TabIndex = 6;
			this->bApply->Text = L"Apply";
			this->bApply->UseVisualStyleBackColor = true;
			this->bApply->Click += gcnew System::EventHandler(this, &Form1::button2_Click);
			// 
			// nudSampleRate
			// 
			this->nudSampleRate->Increment = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->nudSampleRate->Location = System::Drawing::Point(160, 21);
			this->nudSampleRate->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {96000, 0, 0, 0});
			this->nudSampleRate->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {8000, 0, 0, 0});
			this->nudSampleRate->Name = L"nudSampleRate";
			this->nudSampleRate->Size = System::Drawing::Size(120, 20);
			this->nudSampleRate->TabIndex = 0;
			this->nudSampleRate->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {32000, 0, 0, 0});
			// 
			// nudMidiInPort
			// 
			this->nudMidiInPort->Location = System::Drawing::Point(160, 47);
			this->nudMidiInPort->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {16, 0, 0, 0});
			this->nudMidiInPort->Name = L"nudMidiInPort";
			this->nudMidiInPort->Size = System::Drawing::Size(120, 20);
			this->nudMidiInPort->TabIndex = 1;
			// 
			// nudVolume
			// 
			this->nudVolume->Location = System::Drawing::Point(160, 73);
			this->nudVolume->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->nudVolume->Name = L"nudVolume";
			this->nudVolume->Size = System::Drawing::Size(120, 20);
			this->nudVolume->TabIndex = 2;
			this->nudVolume->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {100, 0, 0, 0});
			this->nudVolume->ValueChanged += gcnew System::EventHandler(this, &Form1::nudVolume_ValueChanged);
			// 
			// nudBufferLatency
			// 
			this->nudBufferLatency->Location = System::Drawing::Point(160, 99);
			this->nudBufferLatency->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) {1000, 0, 0, 0});
			this->nudBufferLatency->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) {40, 0, 0, 0});
			this->nudBufferLatency->Name = L"nudBufferLatency";
			this->nudBufferLatency->Size = System::Drawing::Size(120, 20);
			this->nudBufferLatency->TabIndex = 3;
			this->nudBufferLatency->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) {70, 0, 0, 0});
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(41, 23);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(87, 13);
			this->label1->TabIndex = 7;
			this->label1->Text = L"Sample Rate, Hz";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(41, 49);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(64, 13);
			this->label2->TabIndex = 8;
			this->label2->Text = L"MIDI In Port";
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(41, 75);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(77, 13);
			this->label3->TabIndex = 9;
			this->label3->Text = L"Master Volume";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(41, 101);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(95, 13);
			this->label4->TabIndex = 10;
			this->label4->Text = L"Buffer Latency, ms";
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(44, 132);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(86, 13);
			this->label5->TabIndex = 12;
			this->label5->Text = L"DAC Input Mode";
			// 
			// chbReverbEnabled
			// 
			this->chbReverbEnabled->AutoSize = true;
			this->chbReverbEnabled->Checked = true;
			this->chbReverbEnabled->CheckState = System::Windows::Forms::CheckState::Checked;
			this->chbReverbEnabled->Location = System::Drawing::Point(160, 158);
			this->chbReverbEnabled->Name = L"chbReverbEnabled";
			this->chbReverbEnabled->Size = System::Drawing::Size(95, 17);
			this->chbReverbEnabled->TabIndex = 5;
			this->chbReverbEnabled->Text = L"reverbEnabled";
			this->chbReverbEnabled->UseVisualStyleBackColor = true;
			this->chbReverbEnabled->CheckedChanged += gcnew System::EventHandler(this, &Form1::chbReverbEnabled_CheckedChanged);
			// 
			// cbDACInputMode
			// 
			this->cbDACInputMode->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->cbDACInputMode->FormattingEnabled = true;
			this->cbDACInputMode->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"DACInputMode_NICE", L"DACInputMode_PURE", 
				L"DACInputMode_GENERATION1", L"DACInputMode_GENERATION2"});
			this->cbDACInputMode->Location = System::Drawing::Point(159, 124);
			this->cbDACInputMode->Name = L"cbDACInputMode";
			this->cbDACInputMode->Size = System::Drawing::Size(121, 21);
			this->cbDACInputMode->TabIndex = 4;
			this->cbDACInputMode->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::cbDACInputMode_SelectedIndexChanged);
			// 
			// chbShowConsole
			// 
			this->chbShowConsole->AutoSize = true;
			this->chbShowConsole->Location = System::Drawing::Point(28, 158);
			this->chbShowConsole->Name = L"chbShowConsole";
			this->chbShowConsole->Size = System::Drawing::Size(94, 17);
			this->chbShowConsole->TabIndex = 5;
			this->chbShowConsole->Text = L"Show Console";
			this->chbShowConsole->UseVisualStyleBackColor = true;
			this->chbShowConsole->CheckedChanged += gcnew System::EventHandler(this, &Form1::chbShowConsole_CheckedChanged);
			// 
			// Form1
			// 
			this->AcceptButton = this->bApply;
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->CancelButton = this->bClose;
			this->ClientSize = System::Drawing::Size(292, 227);
			this->Controls->Add(this->cbDACInputMode);
			this->Controls->Add(this->chbShowConsole);
			this->Controls->Add(this->chbReverbEnabled);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->nudBufferLatency);
			this->Controls->Add(this->nudVolume);
			this->Controls->Add(this->nudMidiInPort);
			this->Controls->Add(this->nudSampleRate);
			this->Controls->Add(this->bApply);
			this->Controls->Add(this->bReset);
			this->Controls->Add(this->bClose);
			this->Name = L"Form1";
			this->Text = L"MT32Emu";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudSampleRate))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudMidiInPort))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudVolume))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->nudBufferLatency))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
				this->Close();
			 }
	private: System::Void button1_Click_1(System::Object^  sender, System::EventArgs^  e) {
				midiSynth.Reset();
			 }
	private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {
				midiSynth.Close();
				midiSynth.SetParameters((UINT)nudSampleRate->Value, (UINT)nudMidiInPort->Value,
					 (UINT)nudBufferLatency->Value);
				midiSynth.Init();
			 }
private: System::Void nudVolume_ValueChanged(System::Object^  sender, System::EventArgs^  e) {
			midiSynth.SetMasterVolume((UINT)(nudVolume->Value));
		 }
private: System::Void chbReverbEnabled_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			midiSynth.SetReverbEnabled(chbReverbEnabled->Checked);
		 }
private: System::Void cbDACInputMode_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			midiSynth.SetDACInputMode((MT32Emu::DACInputMode)cbDACInputMode->SelectedIndex);
		 }
private: System::Void chbShowConsole_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			HWND h = GetConsoleWindow();
			if (chbShowConsole->Checked) {
				ShowWindow(h, SW_RESTORE);
			} else {
				ShowWindow(h, SW_HIDE);
			}
		 }
};
}