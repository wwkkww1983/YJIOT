package com.yjyz.iot.sms;

public class SMSResult {
	private String factory;
	private String code;
	private String message;

	private String verCode;

	public String getFactory() {
		return factory;
	}

	public void setFactory(String factory) {
		this.factory = factory;
	}

	public String getCode() {
		return code;
	}

	public void setCode(String code) {
		this.code = code;
	}

	public String getMessage() {
		return message;
	}

	public void setMessage(String message) {
		this.message = message;
	}

	public SMSResult(String factory, String code, String message) {
		this.message = message;
		this.factory = factory;
		this.code = code;
	}

	public String getVerCode() {
		return verCode;
	}

	public void setVerCode(String verCode) {
		this.verCode = verCode;
	}
}
