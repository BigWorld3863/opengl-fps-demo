#pragma once

#include "Model.h"

struct Ray 
{
	vmath::vec3 origin;
	vmath::vec3 direction;
};


class GunModel : public Model
{
public:
	bool fullAuto; // 발사 모드: false = 반자동, true = 자동
	float recoilOffset = 0.0f; // 총기 모델 반동 위치 보정값
	float recoilRecoverySpeed; // 총기 모델 반동 회복 속도
	bool isRecoiling;
	float cameraRecoilOffset = 0.0f;	// 카메라 반동 위치 보정값
	float cameraRecoilStrength = 1.0f;       // 카메라 반동 강도
	float cameraRecoilRecoverySpeed = 4.0f;      // 카메라 반동 회복 속도

	bool isReloading; // 현재 장전 중인지 여부
	float reloadTime; // 장전에 걸리는 시간
	int maxAmmo; // 탄창 최대 탄 수
	int currentAmmo; // 현재 탄창에 남은 탄 수

	GunModel(bool AutoMode = false, int maxAmmo = 30, int currentAmmo = 30, bool isReload = false, float rTime = 3.0f, float rAmount = 0.2f,
		float rrs=0.7f, bool iR=false, float RPM = 0.1f, float cRS = 2.0f)
		: fullAuto(AutoMode), maxAmmo(maxAmmo), currentAmmo(currentAmmo), isReloading(isReload), reloadTime(rTime), recoilAmount(rAmount),
		recoilRecoverySpeed(rrs), isRecoiling(iR), firingSpeed(RPM), cameraRecoilStrength(cRS) {}

	bool shoot(float currentTime)
	{
		if (isReloading || currentAmmo <= 0)
			return false;

		//printf("shoot!\n");
		//return true;
		if (currentTime - lastShotTime >= firingSpeed) {
			currentAmmo--;
			lastShotTime = currentTime;
			isRecoiling = true;
			recoilOffset = recoilAmount;

			cameraRecoilOffset += cameraRecoilStrength;

			return true;
		}
		return false;
	}


private:
	float recoilAmount; // 총기 모델 반동량
	float firingSpeed; // 발사 간 최소 시간 간격
	float lastShotTime = 0.0f;
	
};
