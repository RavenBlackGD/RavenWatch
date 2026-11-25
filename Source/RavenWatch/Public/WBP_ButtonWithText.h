// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WBP_ButtonWithText.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWidgetClickResponseSignature);
/**
 * 
 */
UCLASS()
class RAVENWATCH_API UWBP_ButtonWithText : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Info")
	FText Info;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Info")
	FSlateFontInfo InfoFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Info")
	FSlateColor InfoColor;

	UPROPERTY(BlueprintAssignable, Category = "Button Events")
	FOnWidgetClickResponseSignature WidgetClickResponceEvent;

	void FocusInnerButton();
protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleClickEvent();


	UPROPERTY(meta = (BindWidget))
	UButton* BackgroundButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* Information;
};
